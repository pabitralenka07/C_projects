#include "contact.h"
#include "safe_utils.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void menu_add_contact(ContactBook *book) {
    printf("\n--- Add New Contact ---\n");

    char name[CONTACT_NAME_LEN];
    if (!safe_read_string("Enter Name: ", name, sizeof(name), false)) return;

    char phone[CONTACT_PHONE_LEN];
    if (!safe_read_string("Enter Phone: ", phone, sizeof(phone), false)) return;

    char email[CONTACT_EMAIL_LEN];
    if (!safe_read_string("Enter Email: ", email, sizeof(email), false)) return;

    char address[CONTACT_ADDR_LEN];
    if (!safe_read_string("Enter Address (or press Enter to skip): ", address, sizeof(address), true)) return;

    char category[CONTACT_CAT_LEN];
    if (!safe_read_string("Enter Category (e.g. Work, Family, Friends) [General]: ", category, sizeof(category), true)) return;

    int id = contact_add(book, name, phone, email, address, category);
    if (id > 0) {
        printf("\n  [SUCCESS] Contact added successfully with ID #%d!\n", id);
        contact_book_save(book);
    } else {
        printf("\n  [FAILED] Could not add contact due to validation error or duplicate record.\n");
    }
}

static void menu_edit_contact(ContactBook *book) {
    printf("\n--- Edit Contact ---\n");

    int id = 0;
    if (!safe_read_int("Enter Contact ID to edit: ", &id, 1, 999999)) return;

    Contact *c = contact_find_by_id(book, id);
    if (!c) {
        printf("  Contact with ID #%d not found.\n", id);
        return;
    }

    printf("\nEditing Contact #%d (%s):\n", c->id, c->name);
    printf("Leave blank (press Enter) to keep existing value.\n\n");

    char name[CONTACT_NAME_LEN];
    printf("Current Name [%s]: ", c->name);
    if (!safe_read_string("", name, sizeof(name), true)) return;

    char phone[CONTACT_PHONE_LEN];
    printf("Current Phone [%s]: ", c->phone);
    if (!safe_read_string("", phone, sizeof(phone), true)) return;

    char email[CONTACT_EMAIL_LEN];
    printf("Current Email [%s]: ", c->email);
    if (!safe_read_string("", email, sizeof(email), true)) return;

    char address[CONTACT_ADDR_LEN];
    printf("Current Address [%s]: ", c->address);
    if (!safe_read_string("", address, sizeof(address), true)) return;

    char category[CONTACT_CAT_LEN];
    printf("Current Category [%s]: ", c->category);
    if (!safe_read_string("", category, sizeof(category), true)) return;

    if (contact_update(book, id,
                       name[0] ? name : NULL,
                       phone[0] ? phone : NULL,
                       email[0] ? email : NULL,
                       address[0] ? address : NULL,
                       category[0] ? category : NULL)) {
        printf("\n  [SUCCESS] Contact #%d updated successfully!\n", id);
        contact_book_save(book);
    } else {
        printf("\n  [FAILED] Contact update failed.\n");
    }
}

static void menu_delete_contact(ContactBook *book) {
    printf("\n--- Delete Contact ---\n");

    int id = 0;
    if (!safe_read_int("Enter Contact ID to delete: ", &id, 1, 999999)) return;

    Contact *c = contact_find_by_id(book, id);
    if (!c) {
        printf("  Contact with ID #%d not found.\n", id);
        return;
    }

    printf("\nContact to delete: #%d - %s (%s, %s)\n", c->id, c->name, c->phone, c->email);

    /* Confirmation prompt: EOF must mean "no" */
    if (safe_confirm("Are you sure you want to delete this contact?")) {
        contact_delete(book, id);
        printf("  [SUCCESS] Contact #%d has been deleted.\n", id);
        contact_book_save(book);
    } else {
        printf("  Deletion canceled.\n");
    }
}

static void menu_search_contacts(const ContactBook *book) {
    printf("\n--- Search Contacts ---\n");

    char query[64];
    if (!safe_read_string("Enter search term (name, phone, or email): ", query, sizeof(query), false)) return;

    int results[64];
    int count = contact_search(book, query, results, 64);

    if (count == 0) {
        printf("\n  No contacts found matching '%s'.\n", query);
        return;
    }

    printf("\nFound %d matching contact(s):\n", count);
    printf("  +----+----------------------+-------------------+-----------------------+------------+\n");
    printf("  | ID | Name                 | Phone             | Email                 | Category   |\n");
    printf("  +----+----------------------+-------------------+-----------------------+------------+\n");

    for (int i = 0; i < count; i++) {
        const Contact *c = contact_find_by_id((ContactBook *)book, results[i]);
        if (c) {
            printf("  | %-2d | %-20.20s | %-17.17s | %-21.21s | %-10.10s |\n",
                   c->id, c->name, c->phone, c->email, c->category);
        }
    }
    printf("  +----+----------------------+-------------------+-----------------------+------------+\n");
}

static void menu_list_contacts(ContactBook *book) {
    printf("\n--- Contact List ---\n");
    printf("  1. Sort by Name (A-Z)\n");
    printf("  2. Sort by Name (Z-A)\n");
    printf("  3. Sort by ID (Original Order)\n");

    int sort_choice = 0;
    if (safe_read_int("Select sorting option [1-3]: ", &sort_choice, 1, 3)) {
        contact_sort(book, (ContactSortOrder)sort_choice);
    }
    contact_list_all(book);
}

static void menu_export_csv(const ContactBook *book) {
    printf("\n--- Export Contacts to CSV ---\n");

    char path[256];
    if (!safe_read_string("Enter destination CSV file path [e.g. data/contacts.csv]: ",
                          path, sizeof(path), false)) return;

    if (contact_export_csv(book, path)) {
        printf("\n  [SUCCESS] Contacts exported successfully to '%s'!\n", path);
    } else {
        printf("\n  [FAILED] Failed to export contacts to CSV.\n");
    }
}

static void menu_import_csv(ContactBook *book) {
    printf("\n--- Import Contacts from CSV ---\n");

    char path[256];
    if (!safe_read_string("Enter CSV file path to import: ", path, sizeof(path), false)) return;

    int imported = 0;
    int skipped = 0;
    if (contact_import_csv(book, path, &imported, &skipped)) {
        printf("\n  [SUCCESS] CSV Import complete!\n");
        printf("  Contacts Imported: %d\n", imported);
        printf("  Duplicates / Invalid skipped: %d\n", skipped);
        if (imported > 0) {
            contact_book_save(book);
        }
    } else {
        printf("\n  [FAILED] Failed to open CSV file for import.\n");
    }
}

int main(void) {
    ContactBook book;
    contact_book_init(&book);

    if (!contact_book_load(&book)) {
        fprintf(stderr, "Error initializing contact database.\n");
        contact_book_free(&book);
        return 1;
    }

    bool running = true;
    while (running) {
        safe_clear_screen();
        printf("===============================================\n");
        printf("          Contact Management System            \n");
        printf("===============================================\n");
        printf("  1. Add New Contact\n");
        printf("  2. Edit Contact\n");
        printf("  3. Delete Contact\n");
        printf("  4. Search Contacts\n");
        printf("  5. List All Contacts (with Sorting)\n");
        printf("  6. Export Contacts to CSV\n");
        printf("  7. Import Contacts from CSV\n");
        printf("  8. Save & Exit\n");
        printf("===============================================\n");

        int choice = safe_menu_choice(1, 8);
        if (choice == -1 || choice == 8) {
            /* Clean exit on Choice 8 or EOF */
            break;
        }

        switch (choice) {
            case 1: menu_add_contact(&book); break;
            case 2: menu_edit_contact(&book); break;
            case 3: menu_delete_contact(&book); break;
            case 4: menu_search_contacts(&book); break;
            case 5: menu_list_contacts(&book); break;
            case 6: menu_export_csv(&book); break;
            case 7: menu_import_csv(&book); break;
            default: break;
        }

        util_press_enter();
    }

    contact_book_save(&book);
    contact_book_free(&book);
    printf("\nThank you for using Contact Management System. Goodbye!\n");
    return 0;
}
