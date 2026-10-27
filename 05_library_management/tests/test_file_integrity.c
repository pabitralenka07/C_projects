/*
 * Unit tests for file operations and persistence integrity.
 * Exercises: atomic save, validated load, corruption detection
 * (wrong magic, unsupported version, truncated file), and backup recovery.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "library.h"
#include "file_ops.h"
#include "utils.h"

/* Helper to generate a unique temporary test directory */
static void make_temp_dir(char *buf, size_t size) {
    char templ[] = "/tmp/lib_test_XXXXXX";
    char *dir = mkdtemp(templ);
    assert(dir != NULL);
    snprintf(buf, size, "%s", dir);
}

/* Helper to remove directory contents and directory */
static void cleanup_temp_dir(const char *dir) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", dir);
    (void)system(cmd);
}

/* Test 1: Fresh start when directory has no data files */
static void test_fresh_start(void) {
    char test_dir[256];
    make_temp_dir(test_dir, sizeof(test_dir));

    setenv("DATA_DIR", test_dir, 1);
    Library lib;
    library_init(&lib);
    assert(strcmp(lib.data_dir, test_dir) == 0);

    bool ok = library_load(&lib);
    assert(ok);
    assert(lib.book_count == 0);
    assert(lib.member_count == 0);
    assert(lib.txn_count == 0);
    assert(lib.next_book_id == 1);
    assert(lib.next_member_id == 1);
    assert(lib.next_txn_id == 1);

    library_free(&lib);
    cleanup_temp_dir(test_dir);
    printf("  [PASS] test_fresh_start\n");
}

/* Test 2: Save and load roundtrip with multiple records */
static void test_save_load_roundtrip(void) {
    char test_dir[256];
    make_temp_dir(test_dir, sizeof(test_dir));
    setenv("DATA_DIR", test_dir, 1);

    Library lib1;
    library_init(&lib1);
    assert(library_load(&lib1));

    /* Add books */
    int b1 = book_add(&lib1, "The C Programming Language", "K&R", "978-0131103627", 1988, 5);
    int b2 = book_add(&lib1, "Expert C Programming", "Peter van der Linden", "978-0131774292", 1994, 3);
    assert(b1 == 1 && b2 == 2);

    /* Add members */
    int m1 = member_add(&lib1, "Dennis Ritchie", "555-0001", "dennis@bell-labs.com");
    int m2 = member_add(&lib1, "Ken Thompson", "555-0002", "ken@bell-labs.com");
    assert(m1 == 1 && m2 == 2);

    /* Add transaction */
    int t1 = txn_issue_book(&lib1, b1, m1);
    assert(t1 == 1);
    assert(lib1.books[0].copies_available == 4);

    /* Save to disk */
    assert(library_save(&lib1));
    library_free(&lib1);

    /* Load into fresh library */
    Library lib2;
    library_init(&lib2);
    assert(library_load(&lib2));

    assert(lib2.book_count == 2);
    assert(lib2.next_book_id == 3);
    assert(strcmp(lib2.books[0].title, "The C Programming Language") == 0);
    assert(lib2.books[0].copies_available == 4);
    assert(strcmp(lib2.books[1].title, "Expert C Programming") == 0);

    assert(lib2.member_count == 2);
    assert(lib2.next_member_id == 3);
    assert(strcmp(lib2.members[0].name, "Dennis Ritchie") == 0);
    assert(strcmp(lib2.members[1].name, "Ken Thompson") == 0);

    assert(lib2.txn_count == 1);
    assert(lib2.next_txn_id == 2);
    assert(lib2.transactions[0].book_id == b1);
    assert(lib2.transactions[0].member_id == m1);
    assert(!lib2.transactions[0].is_returned);

    library_free(&lib2);
    cleanup_temp_dir(test_dir);
    printf("  [PASS] test_save_load_roundtrip\n");
}

/* Test 3: Detecting corrupted magic header */
static void test_corrupted_magic(void) {
    char test_dir[256];
    make_temp_dir(test_dir, sizeof(test_dir));

    char path[512];
    snprintf(path, sizeof(path), "%s/books.dat", test_dir);

    /* Write file with invalid magic bytes */
    FILE *fp = fopen(path, "wb");
    assert(fp != NULL);
    FileHeader bad_hdr = {
        .magic = {'B', 'A', 'D', '!'},
        .version = FILE_VERSION,
        .record_count = 0,
        .next_id = 1
    };
    fwrite(&bad_hdr, sizeof(bad_hdr), 1, fp);
    fclose(fp);

    void *data = NULL;
    uint32_t count = 0, next_id = 0;
    int res = file_validated_load(path, "LIBb", &data, sizeof(Book), &count, &next_id);
    assert(res == FILE_LOAD_CORRUPT);
    assert(data == NULL);

    cleanup_temp_dir(test_dir);
    printf("  [PASS] test_corrupted_magic\n");
}

/* Test 4: Detecting unsupported version */
static void test_unsupported_version(void) {
    char test_dir[256];
    make_temp_dir(test_dir, sizeof(test_dir));

    char path[512];
    snprintf(path, sizeof(path), "%s/books.dat", test_dir);

    FILE *fp = fopen(path, "wb");
    assert(fp != NULL);
    FileHeader bad_hdr = {
        .magic = {'L', 'I', 'B', 'b'},
        .version = 9999, /* unsupported version */
        .record_count = 0,
        .next_id = 1
    };
    fwrite(&bad_hdr, sizeof(bad_hdr), 1, fp);
    fclose(fp);

    void *data = NULL;
    uint32_t count = 0, next_id = 0;
    int res = file_validated_load(path, "LIBb", &data, sizeof(Book), &count, &next_id);
    assert(res == FILE_LOAD_CORRUPT);
    assert(data == NULL);

    cleanup_temp_dir(test_dir);
    printf("  [PASS] test_unsupported_version\n");
}

/* Test 5: Detecting truncated file (file size smaller than header says) */
static void test_truncated_file(void) {
    char test_dir[256];
    make_temp_dir(test_dir, sizeof(test_dir));

    char path[512];
    snprintf(path, sizeof(path), "%s/books.dat", test_dir);

    FILE *fp = fopen(path, "wb");
    assert(fp != NULL);
    FileHeader hdr = {
        .magic = {'L', 'I', 'B', 'b'},
        .version = FILE_VERSION,
        .record_count = 5, /* Claims 5 records */
        .next_id = 6
    };
    fwrite(&hdr, sizeof(hdr), 1, fp);
    /* Write only 1 record instead of 5 */
    Book single_book;
    memset(&single_book, 0, sizeof(single_book));
    single_book.id = 1;
    strncpy(single_book.title, "Truncated Book", sizeof(single_book.title) - 1);
    fwrite(&single_book, sizeof(single_book), 1, fp);
    fclose(fp);

    void *data = NULL;
    uint32_t count = 0, next_id = 0;
    int res = file_validated_load(path, "LIBb", &data, sizeof(Book), &count, &next_id);
    assert(res == FILE_LOAD_CORRUPT);
    assert(data == NULL);

    cleanup_temp_dir(test_dir);
    printf("  [PASS] test_truncated_file\n");
}

/* Test 6: Backup creation on second save and recovery */
static void test_backup_and_recovery(void) {
    char test_dir[256];
    make_temp_dir(test_dir, sizeof(test_dir));
    setenv("DATA_DIR", test_dir, 1);

    Library lib;
    library_init(&lib);
    assert(library_load(&lib));

    /* First save: writes books.dat */
    book_add(&lib, "Book Version 1", "Author 1", "111-1111111111", 2020, 2);
    assert(library_save(&lib));

    char books_path[512];
    snprintf(books_path, sizeof(books_path), "%s/books.dat", test_dir);
    char bak_path[512];
    snprintf(bak_path, sizeof(bak_path), "%s/books.dat.bak", test_dir);

    /* Second save: backs up v1 to .bak and writes v2 to .dat */
    book_add(&lib, "Book Version 2", "Author 2", "222-2222222222", 2021, 3);
    assert(library_save(&lib));

    /* Verify backup file exists */
    assert(access(bak_path, F_OK) == 0);

    /* Corrupt the main books.dat file */
    FILE *fp = fopen(books_path, "wb");
    assert(fp != NULL);
    fputs("GARBAGE_DATA_CORRUPTION", fp);
    fclose(fp);

    /* Recover from backup */
    bool recovered = file_recover_backup(books_path);
    assert(recovered);

    /* Validate that load succeeds on restored file */
    void *data = NULL;
    uint32_t count = 0, next_id = 0;
    int res = file_validated_load(books_path, "LIBb", &data, sizeof(Book), &count, &next_id);
    assert(res == FILE_LOAD_OK);
    /* Backup had 1 record (the state before the 2nd save) */
    assert(count == 1);
    Book *recovered_books = (Book *)data;
    assert(strcmp(recovered_books[0].title, "Book Version 1") == 0);
    free(data);

    library_free(&lib);
    cleanup_temp_dir(test_dir);
    printf("  [PASS] test_backup_and_recovery\n");
}

int main(void) {
    printf("=== Running File Integrity Tests ===\n");
    test_fresh_start();
    test_save_load_roundtrip();
    test_corrupted_magic();
    test_unsupported_version();
    test_truncated_file();
    test_backup_and_recovery();
    printf("All file integrity tests passed successfully!\n");
    return 0;
}
