/*
 * Airport Management System — Main Entry Point
 * Flight boards, gate assignments, passenger check-in,
 * boarding passes, manifests, and crash-safe binary persistence.
 */

#include "airport.h"
#include "input.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_menu(void) {
    printf("\n");
    util_print_header("Airport Flight & Terminal Management");
    printf("  1. View Flight Status Board\n");
    printf("  2. Passenger Check-In & Boarding Pass\n");
    printf("  3. Book Passenger Ticket\n");
    printf("  4. Update Flight Status (On Time / Delayed / Boarding / Departed)\n");
    printf("  5. Update Flight Gate Assignment\n");
    printf("  6. View Passenger Manifest for Flight\n");
    printf("  7. Search Flights (by Number or City)\n");
    printf("  8. Add New Flight (Admin)\n");
    printf("  9. Exit\n\n");
}

static void do_checkin(Airport *air) {
    printf("\n--- Passenger Check-In ---\n");
    int pid;
    if (!input_read_int("  Enter Passenger ID: ", &pid, 1, 999999)) return;

    Passenger *p = passenger_find_by_id(air, pid);
    if (!p) {
        printf("  Passenger #%d not found.\n", pid);
        return;
    }

    if (p->checked_in) {
        printf("  Passenger is already checked in.\n");
        passenger_print_boarding_pass(air, pid);
        return;
    }

    if (passenger_checkin(air, pid)) {
        printf("\n  Check-in successful! Seat [%02d] confirmed.\n", p->seat_number);
        passenger_print_boarding_pass(air, pid);
        airport_save(air);
    }
}

static void do_book_ticket(Airport *air) {
    printf("\n--- Book Passenger Ticket ---\n");
    flight_list_board(air);

    int fid;
    if (!input_read_int("  Choose Flight ID: ", &fid, 1, 999999)) return;

    Flight *f = flight_find_by_id(air, fid);
    if (!f) {
        printf("  Flight #%d not found.\n", fid);
        return;
    }

    char name[PASS_NAME_LEN], passport[PASSPORT_LEN];
    if (!input_read_string("  Passenger Name: ", name, sizeof(name))) return;
    if (!input_read_string("  Passport Number: ", passport, sizeof(passport))) return;

    int pid = passenger_book(air, name, passport, fid);
    if (pid > 0) {
        printf("\n  Ticket booked successfully! Passenger ID: #%d (Seat: [%02d])\n",
               pid, air->passengers[air->passenger_count - 1].seat_number);
        passenger_print_boarding_pass(air, pid);
        airport_save(air);
    }
}

static void do_update_status(Airport *air) {
    printf("\n--- Update Flight Status ---\n");
    flight_list_board(air);

    int fid;
    if (!input_read_int("  Enter Flight ID: ", &fid, 1, 999999)) return;

    Flight *f = flight_find_by_id(air, fid);
    if (!f) {
        printf("  Flight #%d not found.\n", fid);
        return;
    }

    printf("  Current status: %s\n", f->status);
    printf("  Select status: 1=On Time, 2=Delayed, 3=Boarding, 4=Departed, 5=Cancelled\n");
    int choice;
    if (!input_read_int("  Choice: ", &choice, 1, 5)) return;

    const char *statuses[] = {"On Time", "Delayed", "Boarding", "Departed", "Cancelled"};
    if (flight_update_status(air, fid, statuses[choice - 1])) {
        printf("\n  Flight %s status updated to '%s'!\n", f->flight_number, f->status);
        airport_save(air);
    }
}

static void do_update_gate(Airport *air) {
    int fid, gate;
    if (!input_read_int("  Enter Flight ID: ", &fid, 1, 999999)) return;
    if (!input_read_int("  Enter New Gate Number (1 to 99): ", &gate, 1, 99)) return;

    if (flight_update_gate(air, fid, gate)) {
        printf("\n  Gate updated successfully to Gate %d!\n", gate);
        airport_save(air);
    } else {
        printf("  Flight not found.\n");
    }
}

static void do_manifest(Airport *air) {
    int fid;
    if (!input_read_int("  Enter Flight ID for manifest: ", &fid, 1, 999999)) return;
    passenger_manifest_by_flight(air, fid);
}

static void do_search(Airport *air) {
    char query[64];
    if (!input_read_string("  Search query (flight num, airline, city): ", query, sizeof(query))) return;

    int results[20];
    int count = flight_search(air, query, results, 20);
    printf("\n  Found %d matching flight(s):\n", count);
    for (int i = 0; i < count; i++) {
        Flight *f = flight_find_by_id(air, results[i]);
        if (f) {
            printf("  [%d] %s (%s) %s -> %s | Dept: %s | Gate: %d | Status: %s\n",
                   f->id, f->flight_number, f->airline, f->origin, f->destination,
                   f->departure, f->gate, f->status);
        }
    }
}

static void do_add_flight(Airport *air) {
    printf("\n--- Add New Flight ---\n");
    char num[FLIGHT_NUM_LEN], airline[AIRLINE_NAME_LEN], origin[CITY_NAME_LEN], dest[CITY_NAME_LEN];
    char dept[TIME_STR_LEN], arr[TIME_STR_LEN];
    int gate, cap;

    if (!input_read_string("  Flight Number (e.g. BA-178): ", num, sizeof(num))) return;
    if (!input_read_string("  Airline Name: ", airline, sizeof(airline))) return;
    if (!input_read_string("  Origin City: ", origin, sizeof(origin))) return;
    if (!input_read_string("  Destination City: ", dest, sizeof(dest))) return;
    if (!input_read_string("  Departure Time: ", dept, sizeof(dept))) return;
    if (!input_read_string("  Arrival Time: ", arr, sizeof(arr))) return;
    if (!input_read_int("  Gate: ", &gate, 1, 99)) return;
    if (!input_read_int("  Passenger Capacity: ", &cap, 1, 800)) return;

    int id = flight_add(air, num, airline, origin, dest, dept, arr, gate, cap);
    if (id > 0) {
        printf("\n  Flight %s added successfully! (ID: %d)\n", num, id);
        airport_save(air);
    }
}

int main(void) {
    Airport air;
    airport_init(&air);

    printf("Loading airport records...\n");
    airport_load(&air);

    /* Sample flights seed if starting fresh */
    if (air.flight_count == 0) {
        flight_add(&air, "AA-102", "American Airlines", "New York", "London", "18:00", "06:15", 14, 250);
        flight_add(&air, "DL-450", "Delta Airlines", "Atlanta", "Tokyo", "11:20", "15:45", 8, 300);
        flight_add(&air, "LH-710", "Lufthansa", "Frankfurt", "Singapore", "21:40", "16:20", 22, 350);
        airport_save(&air);
    }
    printf("Ready.\n");

    for (;;) {
        print_menu();
        int choice = input_menu_choice(1, 9);
        if (choice == -1 || choice == 9) break;

        switch (choice) {
        case 1: flight_list_board(&air); break;
        case 2: do_checkin(&air); break;
        case 3: do_book_ticket(&air); break;
        case 4: do_update_status(&air); break;
        case 5: do_update_gate(&air); break;
        case 6: do_manifest(&air); break;
        case 7: do_search(&air); break;
        case 8: do_add_flight(&air); break;
        default: break;
        }

        util_press_enter();
    }

    printf("\nSaving airport records...\n");
    airport_save(&air);
    airport_free(&air);
    printf("Goodbye!\n");
    return EXIT_SUCCESS;
}
