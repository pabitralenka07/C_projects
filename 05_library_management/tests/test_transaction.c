/*
 * Unit tests for the Transaction module.
 * Exercises: issue, return, fine calculation, overdue detection,
 *            copy availability tracking, and validation.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "library.h"

#define SECS_PER_DAY 86400

static Library make_test_lib(void) {
    Library lib;
    memset(&lib, 0, sizeof(lib));
    lib.next_book_id   = 1;
    lib.next_member_id = 1;
    lib.next_txn_id    = 1;
    return lib;
}

/* Helper: create a library with one book (3 copies) and one member */
static Library make_populated_lib(void) {
    Library lib = make_test_lib();
    book_add(&lib, "Test Book", "Test Author", "TST-001", 2024, 3);
    member_add(&lib, "Test Member", "555-0000", "test@test.com");
    return lib;
}

static void test_issue_basic(void) {
    Library lib = make_populated_lib();

    int txn_id = txn_issue_book(&lib, 1, 1);
    assert(txn_id == 1);
    assert(lib.txn_count == 1);
    assert(lib.transactions[0].book_id == 1);
    assert(lib.transactions[0].member_id == 1);
    assert(lib.transactions[0].is_returned == 0);

    /* Book should have one fewer available copy */
    Book *b = book_find_by_id(&lib, 1);
    assert(b->copies_available == 2);

    library_free(&lib);
    printf("  PASS: test_issue_basic\n");
}

static void test_issue_multiple_copies(void) {
    Library lib = make_populated_lib();
    member_add(&lib, "Member 2", "111", "");
    member_add(&lib, "Member 3", "222", "");

    /* Issue all 3 copies */
    assert(txn_issue_book(&lib, 1, 1) > 0);
    assert(txn_issue_book(&lib, 1, 2) > 0);
    assert(txn_issue_book(&lib, 1, 3) > 0);

    Book *b = book_find_by_id(&lib, 1);
    assert(b->copies_available == 0);

    /* Fourth issue should fail — no copies left */
    member_add(&lib, "Member 4", "333", "");
    int txn_id = txn_issue_book(&lib, 1, 4);
    assert(txn_id == -1);

    library_free(&lib);
    printf("  PASS: test_issue_multiple_copies\n");
}

static void test_issue_invalid_book(void) {
    Library lib = make_populated_lib();
    int txn_id = txn_issue_book(&lib, 9999, 1);
    assert(txn_id == -1);
    library_free(&lib);
    printf("  PASS: test_issue_invalid_book\n");
}

static void test_issue_invalid_member(void) {
    Library lib = make_populated_lib();
    int txn_id = txn_issue_book(&lib, 1, 9999);
    assert(txn_id == -1);
    library_free(&lib);
    printf("  PASS: test_issue_invalid_member\n");
}

static void test_return_no_fine(void) {
    Library lib = make_populated_lib();

    int txn_id = txn_issue_book(&lib, 1, 1);
    assert(txn_id > 0);

    /* Return immediately — no fine expected */
    double fine = txn_return_book(&lib, txn_id);
    assert(fine == 0.0);
    assert(lib.transactions[0].is_returned == 1);

    /* Book copy should be restored */
    Book *b = book_find_by_id(&lib, 1);
    assert(b->copies_available == 3);

    library_free(&lib);
    printf("  PASS: test_return_no_fine\n");
}

static void test_return_with_fine(void) {
    Library lib = make_populated_lib();

    int txn_id = txn_issue_book(&lib, 1, 1);
    assert(txn_id > 0);

    /* Backdate the due_date to 5 days ago to simulate overdue */
    lib.transactions[0].due_date = time(NULL) - (5 * SECS_PER_DAY);

    double fine = txn_return_book(&lib, txn_id);
    /* Should be at least $5 (5 days × $1/day) */
    assert(fine >= 5.0);

    library_free(&lib);
    printf("  PASS: test_return_with_fine\n");
}

static void test_return_invalid_txn(void) {
    Library lib = make_populated_lib();
    double fine = txn_return_book(&lib, 9999);
    assert(fine < 0.0);
    library_free(&lib);
    printf("  PASS: test_return_invalid_txn\n");
}

static void test_return_already_returned(void) {
    Library lib = make_populated_lib();

    int txn_id = txn_issue_book(&lib, 1, 1);
    txn_return_book(&lib, txn_id);

    /* Second return should fail */
    double fine = txn_return_book(&lib, txn_id);
    assert(fine < 0.0);

    library_free(&lib);
    printf("  PASS: test_return_already_returned\n");
}

static void test_member_has_active(void) {
    Library lib = make_populated_lib();

    assert(!txn_member_has_active(&lib, 1));

    int txn_id = txn_issue_book(&lib, 1, 1);
    assert(txn_member_has_active(&lib, 1));

    txn_return_book(&lib, txn_id);
    assert(!txn_member_has_active(&lib, 1));

    library_free(&lib);
    printf("  PASS: test_member_has_active\n");
}

static void test_book_issued_count(void) {
    Library lib = make_populated_lib();
    member_add(&lib, "M2", "111", "");

    assert(txn_book_issued_count(&lib, 1) == 0);

    txn_issue_book(&lib, 1, 1);
    assert(txn_book_issued_count(&lib, 1) == 1);

    txn_issue_book(&lib, 1, 2);
    assert(txn_book_issued_count(&lib, 1) == 2);

    library_free(&lib);
    printf("  PASS: test_book_issued_count\n");
}

static void test_validate(void) {
    Transaction valid = {
        .id = 1,
        .book_id = 1,
        .member_id = 1,
        .issue_date = time(NULL),
        .due_date = time(NULL) + (14 * SECS_PER_DAY),
        .return_date = 0,
        .fine = 0.0,
        .is_returned = 0
    };
    assert(txn_validate(&valid));

    Transaction bad_id = valid;
    bad_id.id = 0;
    assert(!txn_validate(&bad_id));

    Transaction bad_dates = valid;
    bad_dates.due_date = bad_dates.issue_date - 1;
    assert(!txn_validate(&bad_dates));

    Transaction bad_fine = valid;
    bad_fine.fine = -1.0;
    assert(!txn_validate(&bad_fine));

    printf("  PASS: test_validate\n");
}

int main(void) {
    printf("Running transaction unit tests...\n");

    test_issue_basic();
    test_issue_multiple_copies();
    test_issue_invalid_book();
    test_issue_invalid_member();
    test_return_no_fine();
    test_return_with_fine();
    test_return_invalid_txn();
    test_return_already_returned();
    test_member_has_active();
    test_book_issued_count();
    test_validate();

    printf("All transaction tests passed!\n");
    return 0;
}
