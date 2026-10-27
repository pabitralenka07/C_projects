#include "library.h"
#include "file_ops.h"
#include "input.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Data file names within the data directory */
#define BOOKS_FILE    "books.dat"
#define MEMBERS_FILE  "members.dat"
#define TXN_FILE      "transactions.dat"

/* Magic identifiers for each data file */
#define BOOKS_MAGIC   "LIBb"
#define MEMBERS_MAGIC "LIBm"
#define TXN_MAGIC     "LIBt"

/* ------------------------------------------------------------------ */
/*  Lifecycle                                                          */
/* ------------------------------------------------------------------ */

void library_init(Library *lib) {
    memset(lib, 0, sizeof(Library));
    lib->next_book_id   = 1;
    lib->next_member_id = 1;
    lib->next_txn_id    = 1;

    /* Use DATA_DIR env var if set, otherwise default to ./data */
    const char *env = getenv("DATA_DIR");
    if (env && env[0] != '\0') {
        snprintf(lib->data_dir, sizeof(lib->data_dir), "%s", env);
    } else {
        snprintf(lib->data_dir, sizeof(lib->data_dir), "./data");
    }
}

void library_free(Library *lib) {
    free(lib->books);
    free(lib->members);
    free(lib->transactions);
    memset(lib, 0, sizeof(Library));
}

/* ------------------------------------------------------------------ */
/*  Load helpers — DRY pattern for each record type                    */
/* ------------------------------------------------------------------ */

/* Generic loader: attempts load, offers backup recovery, validates records.
 * Returns true if the library can continue (even if starting fresh). */
static bool load_records(const char *dir, const char *filename,
                         const char *magic, const char *label,
                         void **out_data, uint32_t *out_count,
                         uint32_t *out_next_id, size_t record_size) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir, filename);

    void *data = NULL;
    uint32_t count = 0, next_id = 1;

    int result = file_validated_load(path, magic, &data, record_size,
                                     &count, &next_id);

    if (result == FILE_LOAD_CORRUPT) {
        printf("\n  WARNING: %s data file is corrupted!\n", label);
        if (input_confirm("  Attempt recovery from backup?", true)) {
            if (file_recover_backup(path)) {
                /* Re-try load after restoring backup */
                result = file_validated_load(path, magic, &data, record_size,
                                             &count, &next_id);
                if (result == FILE_LOAD_OK) {
                    printf("  Recovery successful — loaded %u %s record(s).\n",
                           count, label);
                }
            }
        }
        if (result != FILE_LOAD_OK) {
            printf("  Starting with empty %s database.\n", label);
            data    = NULL;
            count   = 0;
            next_id = 1;
        }
    } else if (result == FILE_LOAD_FRESH) {
        printf("  No %s data file found — starting fresh.\n", label);
    } else if (result == FILE_LOAD_OK && count > 0) {
        printf("  Loaded %u %s record(s).\n", count, label);
    }

    *out_data    = data;
    *out_count   = count;
    *out_next_id = next_id;
    return true;
}

/* ------------------------------------------------------------------ */
/*  Load / Save                                                        */
/* ------------------------------------------------------------------ */

bool library_load(Library *lib) {
    /* Ensure data directory exists */
    if (!util_ensure_dir(lib->data_dir)) {
        fprintf(stderr, "Error: Cannot create data directory '%s'.\n",
                lib->data_dir);
        return false;
    }

    void *data;
    uint32_t count, next_id;

    /* --- Books --- */
    if (!load_records(lib->data_dir, BOOKS_FILE, BOOKS_MAGIC, "books",
                      &data, &count, &next_id, sizeof(Book))) {
        return false;
    }
    lib->books         = (Book *)data;
    lib->book_count    = (int)count;
    lib->book_capacity = (int)count;
    lib->next_book_id  = (int)next_id;

    /* Validate each book record */
    for (int i = 0; i < lib->book_count; i++) {
        if (!book_validate(&lib->books[i])) {
            fprintf(stderr, "  Warning: Book record #%d failed validation — "
                    "marking as deleted.\n", i);
            lib->books[i].is_deleted = 1;
        }
    }

    /* --- Members --- */
    if (!load_records(lib->data_dir, MEMBERS_FILE, MEMBERS_MAGIC, "members",
                      &data, &count, &next_id, sizeof(Member))) {
        return false;
    }
    lib->members         = (Member *)data;
    lib->member_count    = (int)count;
    lib->member_capacity = (int)count;
    lib->next_member_id  = (int)next_id;

    for (int i = 0; i < lib->member_count; i++) {
        if (!member_validate(&lib->members[i])) {
            fprintf(stderr, "  Warning: Member record #%d failed validation — "
                    "marking as deleted.\n", i);
            lib->members[i].is_deleted = 1;
        }
    }

    /* --- Transactions --- */
    if (!load_records(lib->data_dir, TXN_FILE, TXN_MAGIC, "transaction",
                      &data, &count, &next_id, sizeof(Transaction))) {
        return false;
    }
    lib->transactions  = (Transaction *)data;
    lib->txn_count     = (int)count;
    lib->txn_capacity  = (int)count;
    lib->next_txn_id   = (int)next_id;

    for (int i = 0; i < lib->txn_count; i++) {
        if (!txn_validate(&lib->transactions[i])) {
            fprintf(stderr, "  Warning: Transaction record #%d failed validation — "
                    "marking as returned.\n", i);
            lib->transactions[i].is_returned = 1;
        }
    }

    return true;
}

bool library_save(Library *lib) {
    char path[512];

    snprintf(path, sizeof(path), "%s/%s", lib->data_dir, BOOKS_FILE);
    if (!file_atomic_save(path, BOOKS_MAGIC, lib->books, sizeof(Book),
                          (uint32_t)lib->book_count,
                          (uint32_t)lib->next_book_id)) {
        fprintf(stderr, "Error: Failed to save books.\n");
        return false;
    }

    snprintf(path, sizeof(path), "%s/%s", lib->data_dir, MEMBERS_FILE);
    if (!file_atomic_save(path, MEMBERS_MAGIC, lib->members, sizeof(Member),
                          (uint32_t)lib->member_count,
                          (uint32_t)lib->next_member_id)) {
        fprintf(stderr, "Error: Failed to save members.\n");
        return false;
    }

    snprintf(path, sizeof(path), "%s/%s", lib->data_dir, TXN_FILE);
    if (!file_atomic_save(path, TXN_MAGIC, lib->transactions, sizeof(Transaction),
                          (uint32_t)lib->txn_count,
                          (uint32_t)lib->next_txn_id)) {
        fprintf(stderr, "Error: Failed to save transactions.\n");
        return false;
    }

    return true;
}
