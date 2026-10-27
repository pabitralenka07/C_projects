#define _DARWIN_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "contact.h"
#include "safe_utils.h"
#include "file_ops.h"

static void make_temp_dir(char *buf, size_t size) {
    char templ[] = "/tmp/contact_test_XXXXXX";
    char *dir = mkdtemp(templ);
    assert(dir != NULL);
    snprintf(buf, size, "%s", dir);
}

static void cleanup_temp_dir(const char *dir) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", dir);
    (void)system(cmd);
}

static void test_fresh_start(void) {
    char test_dir[256];
    make_temp_dir(test_dir, sizeof(test_dir));
    setenv("DATA_DIR", test_dir, 1);

    ContactBook book;
    contact_book_init(&book);
    assert(contact_book_load(&book));
    assert(book.count == 0);
    assert(book.next_id == 1);

    contact_book_free(&book);
    cleanup_temp_dir(test_dir);
    printf("  [PASS] test_fresh_start\n");
}

static void test_save_load_roundtrip(void) {
    char test_dir[256];
    make_temp_dir(test_dir, sizeof(test_dir));
    setenv("DATA_DIR", test_dir, 1);

    ContactBook b1;
    contact_book_init(&b1);
    assert(contact_book_load(&b1));

    contact_add(&b1, "Linus Torvalds", "555-444-0001", "linus@kernel.org", "Portland", "Linux");
    contact_add(&b1, "Donald Knuth", "555-444-0002", "knuth@stanford.edu", "Stanford", "TeX");
    assert(contact_book_save(&b1));
    contact_book_free(&b1);

    ContactBook b2;
    contact_book_init(&b2);
    assert(contact_book_load(&b2));
    assert(b2.count == 2);
    assert(b2.next_id == 3);

    Contact *c1 = contact_find_by_id(&b2, 1);
    assert(c1 != NULL);
    assert(strcmp(c1->name, "Linus Torvalds") == 0);
    assert(strcmp(c1->phone, "555-444-0001") == 0);

    Contact *c2 = contact_find_by_id(&b2, 2);
    assert(c2 != NULL);
    assert(strcmp(c2->name, "Donald Knuth") == 0);

    contact_book_free(&b2);
    cleanup_temp_dir(test_dir);
    printf("  [PASS] test_save_load_roundtrip\n");
}

static void test_corrupted_magic(void) {
    char test_dir[256];
    make_temp_dir(test_dir, sizeof(test_dir));

    char path[512];
    snprintf(path, sizeof(path), "%s/contacts.dat", test_dir);

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
    int res = file_validated_load(path, CONTACT_MAGIC, &data, sizeof(Contact), &count, &next_id);
    assert(res == FILE_LOAD_CORRUPT);
    assert(data == NULL);

    cleanup_temp_dir(test_dir);
    printf("  [PASS] test_corrupted_magic\n");
}

static void test_unsupported_version(void) {
    char test_dir[256];
    make_temp_dir(test_dir, sizeof(test_dir));

    char path[512];
    snprintf(path, sizeof(path), "%s/contacts.dat", test_dir);

    FILE *fp = fopen(path, "wb");
    assert(fp != NULL);
    FileHeader bad_hdr = {
        .magic = {'C', 'T', 'M', 's'},
        .version = 9999,
        .record_count = 0,
        .next_id = 1
    };
    fwrite(&bad_hdr, sizeof(bad_hdr), 1, fp);
    fclose(fp);

    void *data = NULL;
    uint32_t count = 0, next_id = 0;
    int res = file_validated_load(path, CONTACT_MAGIC, &data, sizeof(Contact), &count, &next_id);
    assert(res == FILE_LOAD_CORRUPT);
    assert(data == NULL);

    cleanup_temp_dir(test_dir);
    printf("  [PASS] test_unsupported_version\n");
}

static void test_truncated_file(void) {
    char test_dir[256];
    make_temp_dir(test_dir, sizeof(test_dir));

    char path[512];
    snprintf(path, sizeof(path), "%s/contacts.dat", test_dir);

    FILE *fp = fopen(path, "wb");
    assert(fp != NULL);
    FileHeader hdr = {
        .magic = {'C', 'T', 'M', 's'},
        .version = FILE_VERSION,
        .record_count = 5, /* Claims 5 records */
        .next_id = 6
    };
    fwrite(&hdr, sizeof(hdr), 1, fp);
    /* Write only 1 record */
    Contact c;
    memset(&c, 0, sizeof(c));
    c.id = 1;
    strncpy(c.name, "Truncated User", sizeof(c.name) - 1);
    fwrite(&c, sizeof(c), 1, fp);
    fclose(fp);

    void *data = NULL;
    uint32_t count = 0, next_id = 0;
    int res = file_validated_load(path, CONTACT_MAGIC, &data, sizeof(Contact), &count, &next_id);
    assert(res == FILE_LOAD_CORRUPT);
    assert(data == NULL);

    cleanup_temp_dir(test_dir);
    printf("  [PASS] test_truncated_file\n");
}

static void test_backup_and_recovery(void) {
    char test_dir[256];
    make_temp_dir(test_dir, sizeof(test_dir));
    setenv("DATA_DIR", test_dir, 1);

    ContactBook book;
    contact_book_init(&book);
    assert(contact_book_load(&book));

    /* Save v1 */
    contact_add(&book, "User One", "555-555-0001", "user1@test.com", "Addr 1", "General");
    assert(contact_book_save(&book));

    char path[512];
    snprintf(path, sizeof(path), "%s/contacts.dat", test_dir);
    char bak_path[512];
    snprintf(bak_path, sizeof(bak_path), "%s/contacts.dat.bak", test_dir);

    /* Save v2 (backs up v1 to .bak) */
    contact_add(&book, "User Two", "555-555-0002", "user2@test.com", "Addr 2", "General");
    assert(contact_book_save(&book));
    assert(access(bak_path, F_OK) == 0);

    /* Corrupt main file */
    FILE *fp = fopen(path, "wb");
    assert(fp != NULL);
    fputs("CORRUPTED_PAYLOAD_GARBAGE", fp);
    fclose(fp);

    /* Verify safe_recover_backup restores v1 */
    bool recovered = safe_recover_backup(path, CONTACT_MAGIC, sizeof(Contact));
    assert(recovered);

    void *data = NULL;
    uint32_t count = 0, next_id = 0;
    int res = file_validated_load(path, CONTACT_MAGIC, &data, sizeof(Contact), &count, &next_id);
    assert(res == FILE_LOAD_OK);
    assert(count == 1);
    Contact *recs = (Contact *)data;
    assert(strcmp(recs[0].name, "User One") == 0);
    free(data);

    contact_book_free(&book);
    cleanup_temp_dir(test_dir);
    printf("  [PASS] test_backup_and_recovery\n");
}

static void test_corrupt_file_does_not_overwrite_good_backup(void) {
    char test_dir[256];
    make_temp_dir(test_dir, sizeof(test_dir));
    setenv("DATA_DIR", test_dir, 1);

    char path[512];
    snprintf(path, sizeof(path), "%s/contacts.dat", test_dir);
    char bak_path[512];
    snprintf(bak_path, sizeof(bak_path), "%s/contacts.dat.bak", test_dir);

    ContactBook book;
    contact_book_init(&book);
    assert(contact_book_load(&book));

    /* 1. Save v1 -> creates contacts.dat */
    contact_add(&book, "Good User", "555-666-0001", "good@test.com", "Addr", "General");
    assert(contact_book_save(&book));

    /* 2. Save v2 -> backs up v1 to contacts.dat.bak */
    contact_add(&book, "Good User 2", "555-666-0002", "good2@test.com", "Addr", "General");
    assert(contact_book_save(&book));

    /* 3. Manually destroy contacts.dat with corrupt garbage */
    FILE *fp = fopen(path, "wb");
    assert(fp != NULL);
    fputs("RANDOM_CORRUPTION_GARBAGE", fp);
    fclose(fp);

    /* 4. Now perform another safe_atomic_save with new in-memory records.
     * safe_atomic_save MUST notice that 'contacts.dat' is corrupt and
     * MUST NOT overwrite the healthy 'contacts.dat.bak'! */
    Contact new_rec;
    memset(&new_rec, 0, sizeof(new_rec));
    new_rec.id = 10;
    strncpy(new_rec.name, "New User", sizeof(new_rec.name) - 1);
    strncpy(new_rec.phone, "555-777-0001", sizeof(new_rec.phone) - 1);
    strncpy(new_rec.email, "new@test.com", sizeof(new_rec.email) - 1);

    bool save_ok = safe_atomic_save(path, CONTACT_MAGIC, &new_rec, sizeof(Contact), 1, 11);
    assert(save_ok);

    /* 5. Verify contacts.dat.bak still has valid header and v1 record count (1 record) */
    void *bak_data = NULL;
    uint32_t bak_count = 0, bak_next = 0;
    int bak_res = file_validated_load(bak_path, CONTACT_MAGIC, &bak_data, sizeof(Contact), &bak_count, &bak_next);
    assert(bak_res == FILE_LOAD_OK);
    assert(bak_count == 1);
    Contact *c_bak = (Contact *)bak_data;
    assert(strcmp(c_bak[0].name, "Good User") == 0);
    free(bak_data);

    contact_book_free(&book);
    cleanup_temp_dir(test_dir);
    printf("  [PASS] test_corrupt_file_does_not_overwrite_good_backup\n");
}

int main(void) {
    printf("=== Running Contact File Integrity Tests ===\n");
    test_fresh_start();
    test_save_load_roundtrip();
    test_corrupted_magic();
    test_unsupported_version();
    test_truncated_file();
    test_backup_and_recovery();
    test_corrupt_file_does_not_overwrite_good_backup();
    printf("All contact file integrity tests passed successfully!\n");
    return 0;
}
