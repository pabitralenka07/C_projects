#include "member.h"
#include "library.h"
#include "transaction.h"
#include "input.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ------------------------------------------------------------------ */
/*  Internal helpers                                                   */
/* ------------------------------------------------------------------ */

static bool ensure_member_capacity(Library *lib) {
    if (lib->member_count < lib->member_capacity) {
        return true;
    }

    int new_cap = (lib->member_capacity == 0) ? 16 : lib->member_capacity * 2;
    Member *grown = realloc(lib->members, (size_t)new_cap * sizeof(Member));
    if (!grown) {
        fprintf(stderr, "Error: Memory allocation failed for members.\n");
        return false;
    }

    lib->members = grown;
    lib->member_capacity = new_cap;
    return true;
}

static bool str_contains_ci(const char *haystack, const char *needle) {
    size_t hlen = strlen(haystack);
    size_t nlen = strlen(needle);
    if (nlen == 0) return true;
    if (nlen > hlen) return false;

    for (size_t i = 0; i <= hlen - nlen; i++) {
        bool match = true;
        for (size_t j = 0; j < nlen; j++) {
            if (tolower((unsigned char)haystack[i + j]) !=
                tolower((unsigned char)needle[j])) {
                match = false;
                break;
            }
        }
        if (match) return true;
    }
    return false;
}

/* ------------------------------------------------------------------ */
/*  CRUD operations                                                    */
/* ------------------------------------------------------------------ */

int member_add(Library *lib, const char *name, const char *phone, const char *email) {
    if (!ensure_member_capacity(lib)) {
        return -1;
    }

    Member *m = &lib->members[lib->member_count];
    memset(m, 0, sizeof(Member));

    m->id = lib->next_member_id++;
    snprintf(m->name,  sizeof(m->name),  "%s", name);
    snprintf(m->phone, sizeof(m->phone), "%s", phone ? phone : "");
    snprintf(m->email, sizeof(m->email), "%s", email ? email : "");
    m->is_deleted = 0;

    lib->member_count++;
    return m->id;
}

Member *member_find_by_id(Library *lib, int id) {
    for (int i = 0; i < lib->member_count; i++) {
        if (lib->members[i].id == id && !lib->members[i].is_deleted) {
            return &lib->members[i];
        }
    }
    return NULL;
}

int member_search(Library *lib, const char *query, int *results, int max_results) {
    int found = 0;
    for (int i = 0; i < lib->member_count && found < max_results; i++) {
        if (lib->members[i].is_deleted) continue;

        if (str_contains_ci(lib->members[i].name,  query) ||
            str_contains_ci(lib->members[i].phone, query) ||
            str_contains_ci(lib->members[i].email, query)) {
            results[found++] = lib->members[i].id;
        }
    }
    return found;
}

bool member_delete(Library *lib, int id) {
    Member *m = member_find_by_id(lib, id);
    if (!m) return false;
    m->is_deleted = 1;
    return true;
}

bool member_update(Library *lib, int id, const char *name,
                   const char *phone, const char *email) {
    Member *m = member_find_by_id(lib, id);
    if (!m) return false;

    snprintf(m->name,  sizeof(m->name),  "%s", name);
    snprintf(m->phone, sizeof(m->phone), "%s", phone ? phone : "");
    snprintf(m->email, sizeof(m->email), "%s", email ? email : "");
    return true;
}

/* ------------------------------------------------------------------ */
/*  Display                                                            */
/* ------------------------------------------------------------------ */

void member_list_all(Library *lib) {
    printf("\n");
    util_print_separator(76);
    printf("  %-4s | %-25s | %-14s | %s\n",
           "ID", "Name", "Phone", "Email");
    util_print_separator(76);

    int count = 0;
    for (int i = 0; i < lib->member_count; i++) {
        if (lib->members[i].is_deleted) continue;
        const Member *m = &lib->members[i];
        printf("  %-4d | %-25.25s | %-14.14s | %.30s\n",
               m->id, m->name, m->phone, m->email);
        count++;
    }

    util_print_separator(76);
    printf("  Total: %d member(s)\n", count);
}

void member_print(const Member *member) {
    printf("  ID:    %d\n",  member->id);
    printf("  Name:  %s\n",  member->name);
    printf("  Phone: %s\n",  member->phone[0] ? member->phone : "(none)");
    printf("  Email: %s\n",  member->email[0] ? member->email : "(none)");
}

bool member_validate(const Member *member) {
    if (member->id <= 0) return false;
    if (member->name[0] == '\0') return false;
    if (member->name[MEMBER_NAME_LEN - 1]   != '\0') return false;
    if (member->phone[MEMBER_PHONE_LEN - 1] != '\0') return false;
    if (member->email[MEMBER_EMAIL_LEN - 1] != '\0') return false;
    return true;
}

/* ------------------------------------------------------------------ */
/*  Interactive menu                                                   */
/* ------------------------------------------------------------------ */

void member_menu(Library *lib) {
    for (;;) {
        printf("\n");
        util_print_header("Member Management");
        printf("  1. Add Member\n");
        printf("  2. List All Members\n");
        printf("  3. Search Members\n");
        printf("  4. View Member Details\n");
        printf("  5. Update Member\n");
        printf("  6. Delete Member\n");
        printf("  7. Back to Main Menu\n\n");

        int choice = input_menu_choice(1, 7);
        if (choice == -1 || choice == 7) return;

        switch (choice) {
        case 1: { /* Add */
            printf("\n--- Add New Member ---\n");
            char name[MEMBER_NAME_LEN], phone[MEMBER_PHONE_LEN];
            char email[MEMBER_EMAIL_LEN];

            if (!input_read_string("  Name: ", name, sizeof(name))) break;
            if (!input_read_string("  Phone: ", phone, sizeof(phone))) break;

            printf("  Email (Enter to skip): ");
            fflush(stdout);
            if (!input_read_line(email, sizeof(email), stdin)) break;
            input_trim(email);

            int id = member_add(lib, name, phone, email);
            if (id > 0) {
                printf("\n  Member added successfully! (ID: %d)\n", id);
                library_save(lib);
            }
            break;
        }
        case 2:
            member_list_all(lib);
            break;

        case 3: { /* Search */
            char query[128];
            if (!input_read_string("  Search (name/phone/email): ", query, sizeof(query)))
                break;

            int results[50];
            int found = member_search(lib, query, results, 50);
            if (found == 0) {
                printf("  No members found matching '%s'.\n", query);
            } else {
                printf("\n  Found %d member(s):\n", found);
                for (int i = 0; i < found; i++) {
                    Member *m = member_find_by_id(lib, results[i]);
                    if (m) {
                        printf("  [%d] %s (%s)\n", m->id, m->name, m->phone);
                    }
                }
            }
            break;
        }
        case 4: { /* View */
            int id;
            if (!input_read_int("  Enter Member ID: ", &id, 1, 999999)) break;
            Member *m = member_find_by_id(lib, id);
            if (m) {
                printf("\n");
                member_print(m);

                /* Also show active issues */
                if (txn_member_has_active(lib, id)) {
                    printf("\n  Active book issues:\n");
                    txn_list_by_member(lib, id);
                }
            } else {
                printf("  Member with ID %d not found.\n", id);
            }
            break;
        }
        case 5: { /* Update */
            int id;
            if (!input_read_int("  Enter Member ID to update: ", &id, 1, 999999)) break;
            Member *m = member_find_by_id(lib, id);
            if (!m) {
                printf("  Member with ID %d not found.\n", id);
                break;
            }

            printf("\n  Current details:\n");
            member_print(m);
            printf("\n  Enter new values (press Enter to keep current):\n");

            char name[MEMBER_NAME_LEN], phone[MEMBER_PHONE_LEN];
            char email[MEMBER_EMAIL_LEN];

            printf("  Name [%s]: ", m->name);
            fflush(stdout);
            if (!input_read_line(name, sizeof(name), stdin)) break;
            input_trim(name);
            if (name[0] == '\0') snprintf(name, sizeof(name), "%s", m->name);

            printf("  Phone [%s]: ", m->phone);
            fflush(stdout);
            if (!input_read_line(phone, sizeof(phone), stdin)) break;
            input_trim(phone);
            if (phone[0] == '\0') snprintf(phone, sizeof(phone), "%s", m->phone);

            printf("  Email [%s]: ", m->email[0] ? m->email : "(none)");
            fflush(stdout);
            if (!input_read_line(email, sizeof(email), stdin)) break;
            input_trim(email);
            if (email[0] == '\0') snprintf(email, sizeof(email), "%s", m->email);

            if (member_update(lib, id, name, phone, email)) {
                printf("\n  Member updated successfully!\n");
                library_save(lib);
            }
            break;
        }
        case 6: { /* Delete */
            int id;
            if (!input_read_int("  Enter Member ID to delete: ", &id, 1, 999999)) break;
            Member *m = member_find_by_id(lib, id);
            if (!m) {
                printf("  Member with ID %d not found.\n", id);
                break;
            }

            /* Prevent deleting members with unreturned books */
            if (txn_member_has_active(lib, id)) {
                printf("  Cannot delete: member has unreturned books.\n");
                break;
            }

            printf("  About to delete: %s\n", m->name);
            if (input_confirm("  Are you sure?", false)) {
                if (member_delete(lib, id)) {
                    printf("  Member deleted successfully.\n");
                    library_save(lib);
                }
            } else {
                printf("  Deletion cancelled.\n");
            }
            break;
        }
        default:
            break;
        }

        util_press_enter();
    }
}
