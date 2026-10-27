/*
 * Unit tests for the Member module.
 * Exercises: add, find, search, delete, update, validation.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "library.h"

static Library make_test_lib(void) {
    Library lib;
    memset(&lib, 0, sizeof(lib));
    lib.next_book_id   = 1;
    lib.next_member_id = 1;
    lib.next_txn_id    = 1;
    return lib;
}

static void test_add_basic(void) {
    Library lib = make_test_lib();

    int id = member_add(&lib, "Alice Johnson", "555-0101", "alice@example.com");
    assert(id == 1);
    assert(lib.member_count == 1);
    assert(strcmp(lib.members[0].name, "Alice Johnson") == 0);
    assert(strcmp(lib.members[0].phone, "555-0101") == 0);
    assert(lib.members[0].is_deleted == 0);

    library_free(&lib);
    printf("  PASS: test_add_basic\n");
}

static void test_add_multiple(void) {
    Library lib = make_test_lib();

    int id1 = member_add(&lib, "Alice", "111", "a@a.com");
    int id2 = member_add(&lib, "Bob",   "222", "b@b.com");
    int id3 = member_add(&lib, "Carol", "333", "");

    assert(id1 == 1 && id2 == 2 && id3 == 3);
    assert(lib.member_count == 3);
    assert(lib.next_member_id == 4);

    library_free(&lib);
    printf("  PASS: test_add_multiple\n");
}

static void test_find_by_id(void) {
    Library lib = make_test_lib();

    int id = member_add(&lib, "Test", "000", "");
    assert(member_find_by_id(&lib, id) != NULL);
    assert(member_find_by_id(&lib, 9999) == NULL);

    library_free(&lib);
    printf("  PASS: test_find_by_id\n");
}

static void test_search(void) {
    Library lib = make_test_lib();

    member_add(&lib, "Alice Smith",  "555-0101", "alice@x.com");
    member_add(&lib, "Bob Johnson",  "555-0202", "bob@x.com");
    member_add(&lib, "Alice Brown",  "555-0303", "abrown@x.com");

    int results[10];

    /* Search by name */
    int found = member_search(&lib, "alice", results, 10);
    assert(found == 2);

    /* Search by phone */
    found = member_search(&lib, "555-0202", results, 10);
    assert(found == 1);

    /* Search by email */
    found = member_search(&lib, "abrown", results, 10);
    assert(found == 1);

    /* No match */
    found = member_search(&lib, "Zara", results, 10);
    assert(found == 0);

    library_free(&lib);
    printf("  PASS: test_search\n");
}

static void test_soft_delete(void) {
    Library lib = make_test_lib();

    int id = member_add(&lib, "Delete Me", "000", "");
    assert(member_delete(&lib, id));
    assert(member_find_by_id(&lib, id) == NULL);
    assert(lib.member_count == 1);
    assert(lib.members[0].is_deleted == 1);

    library_free(&lib);
    printf("  PASS: test_soft_delete\n");
}

static void test_delete_nonexistent(void) {
    Library lib = make_test_lib();
    assert(!member_delete(&lib, 9999));
    library_free(&lib);
    printf("  PASS: test_delete_nonexistent\n");
}

static void test_update(void) {
    Library lib = make_test_lib();

    int id = member_add(&lib, "Old Name", "000", "old@x.com");
    assert(member_update(&lib, id, "New Name", "111", "new@x.com"));

    Member *m = member_find_by_id(&lib, id);
    assert(m != NULL);
    assert(strcmp(m->name, "New Name") == 0);
    assert(strcmp(m->phone, "111") == 0);
    assert(strcmp(m->email, "new@x.com") == 0);

    library_free(&lib);
    printf("  PASS: test_update\n");
}

static void test_validate(void) {
    Member valid = {
        .id = 1,
        .name = "Test",
        .phone = "000",
        .email = "",
        .is_deleted = 0
    };
    assert(member_validate(&valid));

    Member bad_id = valid;
    bad_id.id = 0;
    assert(!member_validate(&bad_id));

    Member bad_name = valid;
    bad_name.name[0] = '\0';
    assert(!member_validate(&bad_name));

    printf("  PASS: test_validate\n");
}

static void test_capacity_growth(void) {
    Library lib = make_test_lib();

    for (int i = 0; i < 50; i++) {
        char name[64];
        snprintf(name, sizeof(name), "Member %d", i);
        int id = member_add(&lib, name, "000", "");
        assert(id > 0);
    }

    assert(lib.member_count == 50);
    assert(lib.member_capacity >= 50);

    library_free(&lib);
    printf("  PASS: test_capacity_growth\n");
}

int main(void) {
    printf("Running member unit tests...\n");

    test_add_basic();
    test_add_multiple();
    test_find_by_id();
    test_search();
    test_soft_delete();
    test_delete_nonexistent();
    test_update();
    test_validate();
    test_capacity_growth();

    printf("All member tests passed!\n");
    return 0;
}
