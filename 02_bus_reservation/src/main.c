/*
 * Bus Reservation System — Main Entry Point
 * Complete seat map rendering, ticket booking, cancellation,
 * and crash-safe binary persistence.
 */

#include "bus.h"
#include "input.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_menu(void) {
    printf("\n");
    util_print_header("Bus Reservation System");
    printf("  1. View All Buses & Routes\n");
    printf("  2. View Bus Seat Map\n");
    printf("  3. Book a Seat / Ticket\n");
    printf("  4. Cancel a Booking\n");
    printf("  5. Print / View Ticket Receipt\n");
    printf("  6. Search Buses by Route\n");
    printf("  7. Add New Bus Route (Admin)\n");
    printf("  8. Exit\n\n");
}

static void do_view_seat_map(BusSystem *sys) {
    bus_list_all(sys);
    int bid;
    if (!input_read_int("  Enter Bus ID to view seat map: ", &bid, 1, 999999)) return;

    Bus *b = bus_find_by_id(sys, bid);
    if (!b) {
        printf("  Bus #%d not found.\n", bid);
        return;
    }

    bus_render_seat_map(b);
}

static void do_book_seat(BusSystem *sys) {
    printf("\n--- Book a Ticket ---\n");
    bus_list_all(sys);
    int bid;
    if (!input_read_int("  Enter Bus ID to book: ", &bid, 1, 999999)) return;

    Bus *b = bus_find_by_id(sys, bid);
    if (!b) {
        printf("  Bus #%d not found.\n", bid);
        return;
    }

    if (bus_available_seats_count(b) == 0) {
        printf("  Sorry, this bus is completely full!\n");
        return;
    }

    bus_render_seat_map(b);

    int seat;
    if (!input_read_int("  Choose an available seat number: ", &seat, 1, b->total_seats)) return;

    char name[PASS_NAME_LEN], phone[PASS_PHONE_LEN];
    if (!input_read_string("  Passenger Name: ", name, sizeof(name))) return;
    if (!input_read_string("  Passenger Phone: ", phone, sizeof(phone))) return;

    int bkid = booking_create(sys, bid, name, phone, seat);
    if (bkid > 0) {
        printf("\n  Booking confirmed! Ticket ID: #%d (Fare: $%.2f)\n", bkid, b->fare);
        booking_print_ticket(sys, bkid);
        bus_system_save(sys);
    }
}

static void do_cancel_booking(BusSystem *sys) {
    printf("\n--- Cancel Booking ---\n");
    int bkid;
    if (!input_read_int("  Enter Ticket / Booking ID to cancel: ", &bkid, 1, 999999)) return;

    Booking *bk = booking_find_by_id(sys, bkid);
    if (!bk) {
        printf("  Booking #%d not found.\n", bkid);
        return;
    }

    if (bk->is_cancelled) {
        printf("  This booking is already cancelled.\n");
        return;
    }

    booking_print_ticket(sys, bkid);
    if (input_confirm("  Are you sure you want to cancel this booking?", false)) {
        if (booking_cancel(sys, bkid)) {
            printf("\n  Booking #%d cancelled successfully. Seat has been released.\n", bkid);
            bus_system_save(sys);
        }
    } else {
        printf("  Cancellation aborted.\n");
    }
}

static void do_print_ticket(BusSystem *sys) {
    int bkid;
    if (!input_read_int("  Enter Ticket / Booking ID: ", &bkid, 1, 999999)) return;
    booking_print_ticket(sys, bkid);
}

static void do_search_routes(BusSystem *sys) {
    printf("\n--- Search Buses by Route ---\n");
    char from[BUS_CITY_LEN], to[BUS_CITY_LEN];

    printf("  Departure City (Enter to match any): ");
    fflush(stdout);
    if (!input_read_line(from, sizeof(from), stdin)) return;
    input_trim(from);

    printf("  Destination City (Enter to match any): ");
    fflush(stdout);
    if (!input_read_line(to, sizeof(to), stdin)) return;
    input_trim(to);

    int results[20];
    int count = bus_search_route(sys, from, to, results, 20);
    printf("\n  Found %d matching bus route(s):\n", count);
    for (int i = 0; i < count; i++) {
        Bus *b = bus_find_by_id(sys, results[i]);
        if (b) {
            printf("  [%d] %-20s | %s -> %s | Dept: %s | $%.2f | %d seats free\n",
                   b->id, b->name, b->route_from, b->route_to,
                   b->departure_time, b->fare, bus_available_seats_count(b));
        }
    }
}

static void do_add_bus(BusSystem *sys) {
    printf("\n--- Add New Bus Route ---\n");
    char name[BUS_NAME_LEN], from[BUS_CITY_LEN], to[BUS_CITY_LEN], time_str[BUS_TIME_LEN];
    double fare;
    int seats;

    if (!input_read_string("  Bus / Operator Name: ", name, sizeof(name))) return;
    if (!input_read_string("  Departure City: ", from, sizeof(from))) return;
    if (!input_read_string("  Destination City: ", to, sizeof(to))) return;
    if (!input_read_string("  Departure Time (HH:MM): ", time_str, sizeof(time_str))) return;
    if (!input_read_double("  Fare ($): ", &fare, 1.0, 1000.0)) return;
    if (!input_read_int("  Total Seats (up to 60): ", &seats, 4, 60)) return;

    int id = bus_add(sys, name, from, to, time_str, fare, seats);
    if (id > 0) {
        printf("\n  Bus #%d added successfully!\n", id);
        bus_system_save(sys);
    }
}

int main(void) {
    BusSystem sys;
    bus_system_init(&sys);

    printf("Loading bus reservation records...\n");
    bus_system_load(&sys);

    /* If starting fresh, seed standard sample routes */
    if (sys.bus_count == 0) {
        bus_add(&sys, "Express Continental", "New York", "Boston", "07:30", 35.0, 40);
        bus_add(&sys, "Pacific Cruiser", "Los Angeles", "San Francisco", "09:00", 45.0, 40);
        bus_add(&sys, "Midwest Shuttle", "Chicago", "Detroit", "11:15", 28.0, 36);
        bus_system_save(&sys);
    }
    printf("Ready.\n");

    for (;;) {
        print_menu();
        int choice = input_menu_choice(1, 8);
        if (choice == -1 || choice == 8) break;

        switch (choice) {
        case 1: bus_list_all(&sys); break;
        case 2: do_view_seat_map(&sys); break;
        case 3: do_book_seat(&sys); break;
        case 4: do_cancel_booking(&sys); break;
        case 5: do_print_ticket(&sys); break;
        case 6: do_search_routes(&sys); break;
        case 7: do_add_bus(&sys); break;
        default: break;
        }

        util_press_enter();
    }

    printf("\nSaving bus records...\n");
    bus_system_save(&sys);
    bus_system_free(&sys);
    printf("Goodbye!\n");
    return EXIT_SUCCESS;
}
