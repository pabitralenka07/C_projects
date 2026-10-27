/*
 * Library Management System — Main Entry Point
 *
 * Features: books, members, issue/return, fines, search.
 * Data persisted in binary files with atomic saves and backup recovery.
 *
 * Build: make
 * Run:   ./build/library
 */

#include "library.h"
#include "book.h"
#include "member.h"
#include "transaction.h"
#include "input.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

static void print_main_menu(void) {
    printf("\n");
    util_print_header("Library Management System");
    printf("  1. Book Management\n");
    printf("  2. Member Management\n");
    printf("  3. Circulation (Issue / Return)\n");
    printf("  4. Search\n");
    printf("  5. Exit\n\n");
}

/* Unified search across books and members */
static void search_menu(Library *lib) {
    char query[128];
    if (!input_read_string("  Search query: ", query, sizeof(query))) {
        return;
    }

    /* Search books */
    int book_results[50];
    int book_found = book_search(lib, query, book_results, 50);

    /* Search members */
    int member_results[50];
    int member_found = member_search(lib, query, member_results, 50);

    if (book_found == 0 && member_found == 0) {
        printf("\n  No results found for '%s'.\n", query);
        return;
    }

    if (book_found > 0) {
        printf("\n  --- Books (%d match%s) ---\n",
               book_found, book_found == 1 ? "" : "es");
        for (int i = 0; i < book_found; i++) {
            Book *b = book_find_by_id(lib, book_results[i]);
            if (b) {
                printf("  [B%d] \"%s\" by %s (%d avail)\n",
                       b->id, b->title, b->author, b->copies_available);
            }
        }
    }

    if (member_found > 0) {
        printf("\n  --- Members (%d match%s) ---\n",
               member_found, member_found == 1 ? "" : "es");
        for (int i = 0; i < member_found; i++) {
            Member *m = member_find_by_id(lib, member_results[i]);
            if (m) {
                printf("  [M%d] %s (%s)\n", m->id, m->name, m->phone);
            }
        }
    }
}

int main(void) {
    Library lib;
    library_init(&lib);

    printf("Loading library data...\n");
    if (!library_load(&lib)) {
        fprintf(stderr, "Fatal: Could not initialize library data.\n");
        library_free(&lib);
        return EXIT_FAILURE;
    }
    printf("Ready.\n");

    for (;;) {
        print_main_menu();

        int choice = input_menu_choice(1, 5);
        if (choice == -1 || choice == 5) {
            break;
        }

        switch (choice) {
        case 1:
            book_menu(&lib);
            break;
        case 2:
            member_menu(&lib);
            break;
        case 3:
            txn_menu(&lib);
            break;
        case 4:
            search_menu(&lib);
            util_press_enter();
            break;
        default:
            break;
        }
    }

    /* Final save and cleanup */
    printf("\nSaving data...\n");
    if (library_save(&lib)) {
        printf("Data saved successfully. Goodbye!\n");
    } else {
        fprintf(stderr, "Warning: Some data may not have been saved.\n");
    }

    library_free(&lib);
    return EXIT_SUCCESS;
}
