#include "transaction.h"
#include "library.h"
#include "book.h"
#include "member.h"
#include "input.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Seconds per day — used for due-date and fine calculations */
#define SECS_PER_DAY 86400

/* ------------------------------------------------------------------ */
/*  Internal helpers                                                   */
/* ------------------------------------------------------------------ */

static bool ensure_txn_capacity(Library *lib) {
    if (lib->txn_count < lib->txn_capacity) {
        return true;
    }

    int new_cap = (lib->txn_capacity == 0) ? 16 : lib->txn_capacity * 2;
    Transaction *grown = realloc(lib->transactions,
                                 (size_t)new_cap * sizeof(Transaction));
    if (!grown) {
        fprintf(stderr, "Error: Memory allocation failed for transactions.\n");
        return false;
    }

    lib->transactions = grown;
    lib->txn_capacity = new_cap;
    return true;
}

/* Format a time_t into a human-readable date string */
static void format_date(time_t t, char *buf, size_t len) {
    if (t == 0) {
        snprintf(buf, len, "(none)");
        return;
    }
    struct tm *tm = localtime(&t);
    if (tm) {
        strftime(buf, len, "%Y-%m-%d", tm);
    } else {
        snprintf(buf, len, "(invalid)");
    }
}

/* ------------------------------------------------------------------ */
/*  Issue / Return logic                                               */
/* ------------------------------------------------------------------ */

int txn_issue_book(Library *lib, int book_id, int member_id) {
    Book *book = book_find_by_id(lib, book_id);
    if (!book) {
        printf("  Error: Book with ID %d not found.\n", book_id);
        return -1;
    }

    Member *member = member_find_by_id(lib, member_id);
    if (!member) {
        printf("  Error: Member with ID %d not found.\n", member_id);
        return -1;
    }

    if (book->copies_available <= 0) {
        printf("  Error: No copies of \"%s\" are available.\n", book->title);
        return -1;
    }

    if (!ensure_txn_capacity(lib)) {
        return -1;
    }

    Transaction *txn = &lib->transactions[lib->txn_count];
    memset(txn, 0, sizeof(Transaction));

    txn->id          = lib->next_txn_id++;
    txn->book_id     = book_id;
    txn->member_id   = member_id;
    txn->issue_date  = time(NULL);
    txn->due_date    = txn->issue_date + (TXN_LOAN_DAYS * SECS_PER_DAY);
    txn->return_date = 0;
    txn->fine        = 0.0;
    txn->is_returned = 0;

    book->copies_available--;
    lib->txn_count++;

    return txn->id;
}

double txn_return_book(Library *lib, int txn_id) {
    /* Find the transaction */
    Transaction *txn = NULL;
    for (int i = 0; i < lib->txn_count; i++) {
        if (lib->transactions[i].id == txn_id && !lib->transactions[i].is_returned) {
            txn = &lib->transactions[i];
            break;
        }
    }

    if (!txn) {
        printf("  Error: Active transaction with ID %d not found.\n", txn_id);
        return -1.0;
    }

    txn->return_date = time(NULL);
    txn->is_returned = 1;

    /* Calculate fine: $1 per overdue day */
    double fine = 0.0;
    if (txn->return_date > txn->due_date) {
        long overdue_secs = (long)(txn->return_date - txn->due_date);
        int  overdue_days = (int)(overdue_secs / SECS_PER_DAY);
        if (overdue_secs % SECS_PER_DAY > 0) {
            overdue_days++;   /* Partial day counts as full day */
        }
        fine = (double)overdue_days * TXN_FINE_PER_DAY;
    }
    txn->fine = fine;

    /* Restore the book's available copy count */
    Book *book = book_find_by_id(lib, txn->book_id);
    if (book) {
        book->copies_available++;
    }

    return fine;
}

/* ------------------------------------------------------------------ */
/*  Queries                                                            */
/* ------------------------------------------------------------------ */

void txn_list_active(Library *lib) {
    printf("\n");
    util_print_separator(82);
    printf("  %-4s | %-4s | %-25s | %-4s | %-15s | %-10s | %s\n",
           "TxID", "BkID", "Book Title", "MbID", "Member", "Issued", "Due");
    util_print_separator(82);

    int count = 0;
    for (int i = 0; i < lib->txn_count; i++) {
        const Transaction *t = &lib->transactions[i];
        if (t->is_returned) continue;

        char issued[16], due[16];
        format_date(t->issue_date, issued, sizeof(issued));
        format_date(t->due_date,   due,   sizeof(due));

        const char *btitle = "???";
        const char *mname  = "???";
        Book *b = book_find_by_id(lib, t->book_id);
        Member *m = member_find_by_id(lib, t->member_id);
        if (b) btitle = b->title;
        if (m) mname  = m->name;

        printf("  %-4d | %-4d | %-25.25s | %-4d | %-15.15s | %-10s | %s\n",
               t->id, t->book_id, btitle, t->member_id, mname, issued, due);
        count++;
    }

    util_print_separator(82);
    printf("  Total: %d active issue(s)\n", count);
}

void txn_list_by_member(Library *lib, int member_id) {
    char issued[16], due[16], returned[16];

    for (int i = 0; i < lib->txn_count; i++) {
        const Transaction *t = &lib->transactions[i];
        if (t->member_id != member_id) continue;

        format_date(t->issue_date,  issued,   sizeof(issued));
        format_date(t->due_date,    due,      sizeof(due));
        format_date(t->return_date, returned, sizeof(returned));

        const char *btitle = "???";
        Book *b = book_find_by_id(lib, t->book_id);
        if (b) btitle = b->title;

        printf("    Txn #%d: \"%s\" | Issued: %s | Due: %s | Returned: %s",
               t->id, btitle, issued, due, returned);
        if (t->fine > 0.0) {
            printf(" | Fine: $%.2f", t->fine);
        }
        printf("\n");
    }
}

void txn_list_by_book(Library *lib, int book_id) {
    char issued[16], due[16], returned[16];

    for (int i = 0; i < lib->txn_count; i++) {
        const Transaction *t = &lib->transactions[i];
        if (t->book_id != book_id) continue;

        format_date(t->issue_date,  issued,   sizeof(issued));
        format_date(t->due_date,    due,      sizeof(due));
        format_date(t->return_date, returned, sizeof(returned));

        const char *mname = "???";
        Member *m = member_find_by_id(lib, t->member_id);
        if (m) mname = m->name;

        printf("    Txn #%d: %s | Issued: %s | Due: %s | Returned: %s",
               t->id, mname, issued, due, returned);
        if (t->fine > 0.0) {
            printf(" | Fine: $%.2f", t->fine);
        }
        printf("\n");
    }
}

void txn_list_overdue(Library *lib) {
    time_t now = time(NULL);
    char issued[16], due[16];

    printf("\n");
    util_print_separator(82);
    printf("  %-4s | %-25s | %-15s | %-10s | %-10s | %s\n",
           "TxID", "Book Title", "Member", "Issued", "Due", "Days Late");
    util_print_separator(82);

    int count = 0;
    for (int i = 0; i < lib->txn_count; i++) {
        const Transaction *t = &lib->transactions[i];
        if (t->is_returned || now <= t->due_date) continue;

        format_date(t->issue_date, issued, sizeof(issued));
        format_date(t->due_date,   due,    sizeof(due));

        int days_late = (int)((now - t->due_date) / SECS_PER_DAY);

        const char *btitle = "???";
        const char *mname  = "???";
        Book *b = book_find_by_id(lib, t->book_id);
        Member *m = member_find_by_id(lib, t->member_id);
        if (b) btitle = b->title;
        if (m) mname  = m->name;

        printf("  %-4d | %-25.25s | %-15.15s | %-10s | %-10s | %d\n",
               t->id, btitle, mname, issued, due, days_late);
        count++;
    }

    util_print_separator(82);
    if (count == 0) {
        printf("  No overdue books.\n");
    } else {
        printf("  Total: %d overdue issue(s)\n", count);
    }
}

bool txn_member_has_active(Library *lib, int member_id) {
    for (int i = 0; i < lib->txn_count; i++) {
        if (lib->transactions[i].member_id == member_id &&
            !lib->transactions[i].is_returned) {
            return true;
        }
    }
    return false;
}

int txn_book_issued_count(Library *lib, int book_id) {
    int count = 0;
    for (int i = 0; i < lib->txn_count; i++) {
        if (lib->transactions[i].book_id == book_id &&
            !lib->transactions[i].is_returned) {
            count++;
        }
    }
    return count;
}

bool txn_validate(const Transaction *txn) {
    if (txn->id <= 0) return false;
    if (txn->book_id <= 0) return false;
    if (txn->member_id <= 0) return false;
    if (txn->issue_date <= 0) return false;
    if (txn->due_date <= 0) return false;
    if (txn->due_date < txn->issue_date) return false;
    if (txn->fine < 0.0) return false;
    return true;
}

/* ------------------------------------------------------------------ */
/*  Interactive menu                                                   */
/* ------------------------------------------------------------------ */

void txn_menu(Library *lib) {
    for (;;) {
        printf("\n");
        util_print_header("Circulation (Issue / Return)");
        printf("  1. Issue a Book\n");
        printf("  2. Return a Book\n");
        printf("  3. View Active Issues\n");
        printf("  4. View Overdue Books\n");
        printf("  5. View Issues by Member\n");
        printf("  6. View Issues by Book\n");
        printf("  7. Back to Main Menu\n\n");

        int choice = input_menu_choice(1, 7);
        if (choice == -1 || choice == 7) return;

        switch (choice) {
        case 1: { /* Issue */
            printf("\n--- Issue a Book ---\n");
            int book_id, member_id;
            if (!input_read_int("  Book ID: ", &book_id, 1, 999999)) break;
            if (!input_read_int("  Member ID: ", &member_id, 1, 999999)) break;

            int txn_id = txn_issue_book(lib, book_id, member_id);
            if (txn_id > 0) {
                char due[16];
                /* Find the transaction we just created */
                for (int i = lib->txn_count - 1; i >= 0; i--) {
                    if (lib->transactions[i].id == txn_id) {
                        format_date(lib->transactions[i].due_date, due, sizeof(due));
                        break;
                    }
                }
                printf("\n  Book issued! Transaction ID: %d, Due: %s\n", txn_id, due);
                library_save(lib);
            }
            break;
        }
        case 2: { /* Return */
            printf("\n--- Return a Book ---\n");
            int txn_id;
            if (!input_read_int("  Transaction ID: ", &txn_id, 1, 999999)) break;

            double fine = txn_return_book(lib, txn_id);
            if (fine >= 0.0) {
                printf("\n  Book returned successfully!\n");
                if (fine > 0.0) {
                    printf("  Fine: $%.2f (overdue)\n", fine);
                } else {
                    printf("  No fine — returned on time.\n");
                }
                library_save(lib);
            }
            break;
        }
        case 3:
            txn_list_active(lib);
            break;
        case 4:
            txn_list_overdue(lib);
            break;
        case 5: {
            int mid;
            if (!input_read_int("  Member ID: ", &mid, 1, 999999)) break;
            Member *m = member_find_by_id(lib, mid);
            if (!m) {
                printf("  Member with ID %d not found.\n", mid);
                break;
            }
            printf("\n  Transaction history for %s (ID %d):\n", m->name, m->id);
            txn_list_by_member(lib, mid);
            break;
        }
        case 6: {
            int bid;
            if (!input_read_int("  Book ID: ", &bid, 1, 999999)) break;
            Book *b = book_find_by_id(lib, bid);
            if (!b) {
                printf("  Book with ID %d not found.\n", bid);
                break;
            }
            printf("\n  Transaction history for \"%s\" (ID %d):\n", b->title, b->id);
            txn_list_by_book(lib, bid);
            break;
        }
        default:
            break;
        }

        util_press_enter();
    }
}
