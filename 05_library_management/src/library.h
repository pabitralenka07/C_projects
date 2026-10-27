#ifndef LIBRARY_H
#define LIBRARY_H

#include "book.h"
#include "member.h"
#include "transaction.h"
#include <stdbool.h>

/* Central data store — owns all dynamically allocated record arrays. */
struct Library {
    Book        *books;
    int          book_count;
    int          book_capacity;

    Member      *members;
    int          member_count;
    int          member_capacity;

    Transaction *transactions;
    int          txn_count;
    int          txn_capacity;

    int          next_book_id;
    int          next_member_id;
    int          next_txn_id;

    char         data_dir[256];   /* Path to the data directory */
};

/* Zero-initialize and set data_dir from DATA_DIR env var or "./data". */
void library_init(Library *lib);

/* Free all dynamically allocated memory. */
void library_free(Library *lib);

/* Load all data files (books, members, transactions).
 * Offers backup recovery if any file is corrupt.
 * Returns true on success (including fresh start). */
bool library_load(Library *lib);

/* Save all data files atomically. Returns true on success. */
bool library_save(Library *lib);

#endif /* LIBRARY_H */
