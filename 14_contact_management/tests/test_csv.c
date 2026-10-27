#define _DARWIN_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "contact.h"

static void test_csv_export_and_import(void) {
    char templ[] = "/tmp/contact_csv_XXXXXX";
    char *dir = mkdtemp(templ);
    assert(dir != NULL);

    char csv_path[512];
    snprintf(csv_path, sizeof(csv_path), "%s/contacts.csv", dir);

    /* Populate source contact book */
    ContactBook src_book;
    contact_book_init(&src_book);

    contact_add(&src_book, "Grace Hopper", "555-222-0001", "grace@navy.mil",
                "123 Main St, Suite 400", "Navy");
    contact_add(&src_book, "Margaret Hamilton", "555-222-0002", "margaret@mit.edu",
                "Apollo Project, Cambridge, MA", "NASA");

    /* Export to CSV */
    assert(contact_export_csv(&src_book, csv_path));

    /* Verify file exists and has content */
    FILE *fp = fopen(csv_path, "r");
    assert(fp != NULL);
    char buf[512];
    char *header = fgets(buf, sizeof(buf), fp);
    assert(header != NULL);
    assert(strstr(buf, "Name,Phone,Email") != NULL);
    fclose(fp);

    /* Import into fresh book */
    ContactBook dst_book;
    contact_book_init(&dst_book);

    int imported = 0, skipped = 0;
    assert(contact_import_csv(&dst_book, csv_path, &imported, &skipped));
    assert(imported == 2);
    assert(skipped == 0);
    assert(dst_book.count == 2);

    /* Verify records and preserved commas in quoted address field */
    Contact *c1 = contact_find_by_id(&dst_book, 1);
    assert(c1 != NULL);
    assert(strcmp(c1->name, "Grace Hopper") == 0);
    assert(strcmp(c1->phone, "555-222-0001") == 0);
    assert(strcmp(c1->address, "123 Main St, Suite 400") == 0);

    Contact *c2 = contact_find_by_id(&dst_book, 2);
    assert(c2 != NULL);
    assert(strcmp(c2->name, "Margaret Hamilton") == 0);
    assert(strcmp(c2->address, "Apollo Project, Cambridge, MA") == 0);

    /* Second import of same CSV should skip both due to duplicate phone/email */
    int imp2 = 0, skip2 = 0;
    assert(contact_import_csv(&dst_book, csv_path, &imp2, &skip2));
    assert(imp2 == 0);
    assert(skip2 == 2);
    assert(dst_book.count == 2);

    contact_book_free(&src_book);
    contact_book_free(&dst_book);

    char rm_cmd[512];
    snprintf(rm_cmd, sizeof(rm_cmd), "rm -rf \"%s\"", dir);
    (void)system(rm_cmd);

    printf("  [PASS] test_csv_export_and_import\n");
}

static void test_csv_malformed_input(void) {
    char templ[] = "/tmp/contact_bad_csv_XXXXXX";
    char *dir = mkdtemp(templ);
    assert(dir != NULL);

    char csv_path[512];
    snprintf(csv_path, sizeof(csv_path), "%s/malformed.csv", dir);

    FILE *fp = fopen(csv_path, "w");
    assert(fp != NULL);
    fprintf(fp, "Name,Phone,Email,Address,Category\n");
    fprintf(fp, "Valid User,555-333-1111,valid@user.com,Home,General\n");
    fprintf(fp, "\n"); /* Empty line */
    fprintf(fp, "Bad Phone,not-a-number,bad@user.com,Nowhere,Other\n");
    fprintf(fp, "Bad Email,555-333-2222,invalid-email,Nowhere,Other\n");
    fprintf(fp, "Incomplete Line,555-333-3333\n");
    fclose(fp);

    ContactBook book;
    contact_book_init(&book);

    int imported = 0, skipped = 0;
    assert(contact_import_csv(&book, csv_path, &imported, &skipped));
    assert(imported == 1);
    assert(skipped == 2); /* 2 invalid format lines skipped */

    contact_book_free(&book);

    char rm_cmd[512];
    snprintf(rm_cmd, sizeof(rm_cmd), "rm -rf \"%s\"", dir);
    (void)system(rm_cmd);

    printf("  [PASS] test_csv_malformed_input\n");
}

int main(void) {
    printf("=== Running CSV Import/Export Unit Tests ===\n");
    test_csv_export_and_import();
    test_csv_malformed_input();
    printf("All CSV unit tests passed successfully!\n");
    return 0;
}
