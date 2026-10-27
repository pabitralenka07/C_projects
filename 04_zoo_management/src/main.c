/*
 * Zoo Management System — Main Entry Point
 * Animals, enclosures, feeding schedules, staff rosters,
 * gate ticketing, and crash-safe binary persistence.
 */

#include "zoo.h"
#include "input.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_main_menu(void) {
    printf("\n");
    util_print_header("Zoo Wildlife & Visitor Management");
    printf("  1. Animal Registry (Add / List / Search / Move)\n");
    printf("  2. Habitat / Enclosure Management\n");
    printf("  3. Staff & Zookeeper Rosters\n");
    printf("  4. Visitor Gate Ticketing & Revenue\n");
    printf("  5. Exit\n\n");
}

/* ------------------------------------------------------------------ */
/* Animals Submenu                                                    */
/* ------------------------------------------------------------------ */

static void do_animals_menu(Zoo *zoo) {
    for (;;) {
        printf("\n");
        util_print_header("Animal Registry");
        printf("  1. Register New Animal\n");
        printf("  2. List All Animals\n");
        printf("  3. Search Animals by Name or Species\n");
        printf("  4. Transfer Animal to New Enclosure\n");
        printf("  5. Back to Main Menu\n\n");

        int ch = input_menu_choice(1, 5);
        if (ch == -1 || ch == 5) break;

        switch (ch) {
        case 1: {
            printf("\n--- Register New Animal ---\n");
            char name[ANIMAL_NAME_LEN], species[ANIMAL_SPECIES_LEN], feeding[ANIMAL_FEED_LEN];
            int age, enc_id;

            if (!input_read_string("  Animal Name: ", name, sizeof(name))) break;
            if (!input_read_string("  Species (e.g. Panthera leo): ", species, sizeof(species))) break;
            if (!input_read_int("  Age (years): ", &age, 0, 150)) break;

            enclosure_list_all(zoo);
            if (!input_read_int("  Assign Enclosure ID: ", &enc_id, 1, 999999)) break;
            if (!input_read_string("  Feeding Schedule (e.g. 09:00 Meat 5kg): ", feeding, sizeof(feeding))) break;

            int id = animal_add(zoo, name, species, age, enc_id, feeding);
            if (id > 0) {
                printf("\n  Animal registered successfully! (ID: %d)\n", id);
                zoo_save(zoo);
            }
            break;
        }
        case 2:
            animal_list_all(zoo);
            break;
        case 3: {
            char query[64];
            if (!input_read_string("  Search query: ", query, sizeof(query))) break;
            int results[50];
            int count = animal_search(zoo, query, results, 50);
            printf("\n  Found %d matching animal(s):\n", count);
            for (int i = 0; i < count; i++) {
                Animal *a = animal_find_by_id(zoo, results[i]);
                if (a) {
                    printf("  [%d] %s the %s (Age: %d) | Feed: %s\n",
                           a->id, a->name, a->species, a->age, a->feeding_schedule);
                }
            }
            break;
        }
        case 4: {
            int aid, new_eid;
            if (!input_read_int("  Enter Animal ID: ", &aid, 1, 999999)) break;
            enclosure_list_all(zoo);
            if (!input_read_int("  Enter New Enclosure ID: ", &new_eid, 1, 999999)) break;

            if (animal_transfer(zoo, aid, new_eid)) {
                printf("\n  Animal transfer completed successfully!\n");
                zoo_save(zoo);
            } else {
                printf("  Transfer failed: check animal ID or enclosure capacity.\n");
            }
            break;
        }
        default: break;
        }
        util_press_enter();
    }
}

/* ------------------------------------------------------------------ */
/* Enclosures Submenu                                                 */
/* ------------------------------------------------------------------ */

static void do_enclosures_menu(Zoo *zoo) {
    for (;;) {
        printf("\n");
        util_print_header("Enclosure / Habitat Management");
        printf("  1. Add New Enclosure\n");
        printf("  2. List All Enclosures & Occupancies\n");
        printf("  3. Back to Main Menu\n\n");

        int ch = input_menu_choice(1, 3);
        if (ch == -1 || ch == 3) break;

        switch (ch) {
        case 1: {
            printf("\n--- Add New Habitat ---\n");
            char name[ENCLOSURE_NAME_LEN], type[ENCLOSURE_TYPE_LEN];
            int cap;

            if (!input_read_string("  Enclosure Name: ", name, sizeof(name))) break;
            if (!input_read_string("  Type (e.g. Aviary, Savannah, Aquarium): ", type, sizeof(type))) break;
            if (!input_read_int("  Maximum Animal Capacity: ", &cap, 1, 500)) break;

            int id = enclosure_add(zoo, name, type, cap);
            if (id > 0) {
                printf("\n  Enclosure #%d added successfully!\n", id);
                zoo_save(zoo);
            }
            break;
        }
        case 2:
            enclosure_list_all(zoo);
            break;
        default: break;
        }
        util_press_enter();
    }
}

/* ------------------------------------------------------------------ */
/* Staff Submenu                                                      */
/* ------------------------------------------------------------------ */

static void do_staff_menu(Zoo *zoo) {
    for (;;) {
        printf("\n");
        util_print_header("Staff & Zookeeper Management");
        printf("  1. Add Staff Member\n");
        printf("  2. List Staff Roster\n");
        printf("  3. Back to Main Menu\n\n");

        int ch = input_menu_choice(1, 3);
        if (ch == -1 || ch == 3) break;

        switch (ch) {
        case 1: {
            printf("\n--- Add Staff Member ---\n");
            char name[STAFF_NAME_LEN], role[STAFF_ROLE_LEN], phone[STAFF_PHONE_LEN];
            int enc_id;

            if (!input_read_string("  Staff Name: ", name, sizeof(name))) break;
            if (!input_read_string("  Role (Keeper / Veterinarian / Curator / Guide): ", role, sizeof(role))) break;
            if (!input_read_string("  Phone: ", phone, sizeof(phone))) break;

            enclosure_list_all(zoo);
            if (!input_read_int("  Assigned Habitat ID (0 for general): ", &enc_id, 0, 999999)) break;

            int id = staff_add(zoo, name, role, phone, enc_id);
            if (id > 0) {
                printf("\n  Staff member #%d registered successfully!\n", id);
                zoo_save(zoo);
            }
            break;
        }
        case 2:
            staff_list_all(zoo);
            break;
        default: break;
        }
        util_press_enter();
    }
}

/* ------------------------------------------------------------------ */
/* Ticketing Submenu                                                  */
/* ------------------------------------------------------------------ */

static void do_ticketing_menu(Zoo *zoo) {
    for (;;) {
        printf("\n");
        util_print_header("Gate Ticketing & Admissions");
        printf("  1. Sell Admission Tickets\n");
        printf("  2. View Visitor Sales & Revenue Report\n");
        printf("  3. Back to Main Menu\n\n");

        int ch = input_menu_choice(1, 3);
        if (ch == -1 || ch == 3) break;

        switch (ch) {
        case 1: {
            printf("\n--- Sell Tickets ---\n");
            printf("  Ticket Tiers: 1 = Adult ($20.00), 2 = Child ($12.00), 3 = Senior ($15.00)\n");
            int tier, count;
            if (!input_read_int("  Select Tier [1-3]: ", &tier, 1, 3)) break;
            if (!input_read_int("  Number of Tickets: ", &count, 1, 100)) break;

            int tid = ticket_sell(zoo, count, tier);
            if (tid > 0) {
                double rev = zoo->tickets[zoo->ticket_count - 1].revenue;
                printf("\n  Transaction #%d confirmed! Sold %d ticket(s) for $%.2f\n",
                       tid, count, rev);
                zoo_save(zoo);
            }
            break;
        }
        case 2:
            ticket_sales_report(zoo);
            break;
        default: break;
        }
        util_press_enter();
    }
}

int main(void) {
    Zoo zoo;
    zoo_init(&zoo);

    printf("Loading zoo records...\n");
    zoo_load(&zoo);

    /* Sample initial data seed if starting fresh */
    if (zoo.enclosure_count == 0) {
        int e1 = enclosure_add(&zoo, "African Savannah", "Open Plains", 15);
        int e2 = enclosure_add(&zoo, "Amazonian Aviary", "Aviary", 25);
        int e3 = enclosure_add(&zoo, "Reptile Pavilion", "Terrarium", 10);

        animal_add(&zoo, "Simba", "Panthera leo (Lion)", 5, e1, "09:00 Meat 6kg");
        animal_add(&zoo, "Kiki", "Ara macao (Scarlet Macaw)", 3, e2, "08:30 Seeds & Tropical Fruit");
        animal_add(&zoo, "Monty", "Python reticulatus", 7, e3, "Every Wednesday Rodent 2kg");

        staff_add(&zoo, "Jane Goodall", "Head Curator", "555-0150", e1);
        staff_add(&zoo, "Steve Irwin", "Wildlife Keeper", "555-0151", e3);

        zoo_save(&zoo);
    }
    printf("Ready.\n");

    for (;;) {
        print_main_menu();
        int choice = input_menu_choice(1, 5);
        if (choice == -1 || choice == 5) break;

        switch (choice) {
        case 1: do_animals_menu(&zoo); break;
        case 2: do_enclosures_menu(&zoo); break;
        case 3: do_staff_menu(&zoo); break;
        case 4: do_ticketing_menu(&zoo); break;
        default: break;
        }
    }

    printf("\nSaving zoo records...\n");
    zoo_save(&zoo);
    zoo_free(&zoo);
    printf("Goodbye!\n");
    return EXIT_SUCCESS;
}
