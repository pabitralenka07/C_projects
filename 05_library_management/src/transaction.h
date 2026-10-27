#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <stdbool.h>
#include <time.h>

typedef struct Library Library;

#define TXN_LOAN_DAYS    14      /* Books are due after 14 days */
#define TXN_FINE_PER_DAY 1.0     /* $1 per overdue day */

typedef struct {
    int    id;
    int    book_id;
    int    member_id;
    time_t issue_date;
    time_t due_date;            /* issue_date + TXN_LOAN_DAYS days */
    time_t return_date;         /* 0 = not yet returned */
    double fine;                /* Calculated on return: $1/day overdue */
    int    is_returned;         /* 0 = active, 1 = returned */
} Transaction;

/* Issue a book to a member. Returns transaction ID, or -1 on failure. */
int    txn_issue_book(Library *lib, int book_id, int member_id);

/* Return a book. Returns fine amount (0.0 if on time), or -1.0 on error. */
double txn_return_book(Library *lib, int txn_id);

/* List all currently active (unreturned) issues. */
void   txn_list_active(Library *lib);

/* List all transactions for a specific member. */
void   txn_list_by_member(Library *lib, int member_id);

/* List all transactions for a specific book. */
void   txn_list_by_book(Library *lib, int book_id);

/* List all overdue (unreturned past due_date) transactions. */
void   txn_list_overdue(Library *lib);

/* Check whether a member has any unreturned books. */
bool   txn_member_has_active(Library *lib, int member_id);

/* Count how many copies of a book are currently issued. */
int    txn_book_issued_count(Library *lib, int book_id);

/* Validate a Transaction record. */
bool   txn_validate(const Transaction *txn);

/* Interactive sub-menu for issue/return. */
void   txn_menu(Library *lib);

#endif /* TRANSACTION_H */
