/*
 * Unit tests for the Book module.
 * Exercises: add, find, search, delete, update, duplicate-ISBN rejection,
 *            and record validation.
 * Built and run with -fsanitize=address,undefined.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "library.h"

/* Isolated library for each test — no data files involved */
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

    int id = book_add(&lib, "The C Programming Language", "Kernighan & Ritchie",
                      "978-0-13-110362-7", 1988, 3);
    assert(id == 1);
    assert(lib.book_count == 1);
    assert(lib.books[0].copies_total == 3);
    assert(lib.books[0].copies_available == 3);
    assert(strcmp(lib.books[0].title, "The C Programming Language") == 0);
    assert(lib.books[0].is_deleted == 0);

    library_free(&lib);
    printf("  PASS: test_add_basic\n");
}

static void test_add_multiple(void) {
    Library lib = make_test_lib();

    int id1 = book_add(&lib, "Book A", "Author A", "ISBN-A", 2020, 1);
    int id2 = book_add(&lib, "Book B", "Author B", "ISBN-B", 2021, 2);
    int id3 = book_add(&lib, "Book C", "Author C", "",       2022, 5);

    assert(id1 == 1 && id2 == 2 && id3 == 3);
    assert(lib.book_count == 3);
    assert(lib.next_book_id == 4);

    library_free(&lib);
    printf("  PASS: test_add_multiple\n");
}

static void test_duplicate_isbn_rejected(void) {
    Library lib = make_test_lib();

    int id1 = book_add(&lib, "Book A", "Auth A", "SAME-ISBN", 2020, 1);
    assert(id1 > 0);

    /* Same ISBN should be rejected */
    int id2 = book_add(&lib, "Book B", "Auth B", "SAME-ISBN", 2021, 2);
    assert(id2 == -1);
    assert(lib.book_count == 1);

    library_free(&lib);
    printf("  PASS: test_duplicate_isbn_rejected\n");
}

static void test_empty_isbn_no_conflict(void) {
    Library lib = make_test_lib();

    /* Empty ISBNs should not conflict with each other */
    int id1 = book_add(&lib, "Book A", "Auth A", "", 2020, 1);
    int id2 = book_add(&lib, "Book B", "Auth B", "", 2021, 2);
    assert(id1 > 0 && id2 > 0);
    assert(lib.book_count == 2);

    library_free(&lib);
    printf("  PASS: test_empty_isbn_no_conflict\n");
}

static void test_find_by_id(void) {
    Library lib = make_test_lib();

    int id = book_add(&lib, "Test", "Auth", "ISBN-1", 2020, 1);
    Book *found = book_find_by_id(&lib, id);
    assert(found != NULL);
    assert(found->id == id);

    /* Non-existent ID */
    assert(book_find_by_id(&lib, 9999) == NULL);

    library_free(&lib);
    printf("  PASS: test_find_by_id\n");
}

static void test_find_by_isbn(void) {
    Library lib = make_test_lib();

    book_add(&lib, "Test", "Auth", "ISBN-FIND", 2020, 1);
    assert(book_find_by_isbn(&lib, "ISBN-FIND") != NULL);
    assert(book_find_by_isbn(&lib, "NO-SUCH") == NULL);
    assert(book_find_by_isbn(&lib, "") == NULL);

    library_free(&lib);
    printf("  PASS: test_find_by_isbn\n");
}

static void test_search(void) {
    Library lib = make_test_lib();

    book_add(&lib, "Data Structures in C", "Auth A", "", 2020, 1);
    book_add(&lib, "Algorithms",           "Auth B", "", 2021, 1);
    book_add(&lib, "The C Answer Book",    "Auth C", "", 2022, 1);

    int results[10];

    /* Search by title substring */
    int found = book_search(&lib, "C", results, 10);
    assert(found == 2); /* "Data Structures in C" and "The C Answer Book" */

    /* Case-insensitive */
    found = book_search(&lib, "algorithms", results, 10);
    assert(found == 1);

    /* No match */
    found = book_search(&lib, "Python", results, 10);
    assert(found == 0);

    library_free(&lib);
    printf("  PASS: test_search\n");
}

static void test_soft_delete(void) {
    Library lib = make_test_lib();

    int id = book_add(&lib, "Delete Me", "Auth", "DEL-ISBN", 2020, 1);
    assert(book_delete(&lib, id));

    /* Should not be findable after deletion */
    assert(book_find_by_id(&lib, id) == NULL);
    assert(book_find_by_isbn(&lib, "DEL-ISBN") == NULL);

    /* But the record is still in the array (soft delete) */
    assert(lib.book_count == 1);
    assert(lib.books[0].is_deleted == 1);

    /* ISBN should be re-usable after deletion */
    int id2 = book_add(&lib, "New Book", "Auth2", "DEL-ISBN", 2021, 2);
    assert(id2 > 0);

    library_free(&lib);
    printf("  PASS: test_soft_delete\n");
}

static void test_update(void) {
    Library lib = make_test_lib();

    int id = book_add(&lib, "Old Title", "Old Author", "UP-ISBN", 2020, 5);
    assert(book_update(&lib, id, "New Title", "New Author", "UP-ISBN", 2023, 8));

    Book *b = book_find_by_id(&lib, id);
    assert(b != NULL);
    assert(strcmp(b->title, "New Title") == 0);
    assert(strcmp(b->author, "New Author") == 0);
    assert(b->year == 2023);
    assert(b->copies_total == 8);
    /* Available should increase by the same amount (5→8 = +3) */
    assert(b->copies_available == 8);

    library_free(&lib);
    printf("  PASS: test_update\n");
}

static void test_update_preserves_issued(void) {
    Library lib = make_test_lib();

    int id = book_add(&lib, "Book", "Auth", "ISBN-X", 2020, 5);
    Book *b = book_find_by_id(&lib, id);
    /* Simulate 2 copies issued */
    b->copies_available = 3;

    /* Increase total from 5 to 7 — available should go from 3 to 5 (2 still issued) */
    assert(book_update(&lib, id, "Book", "Auth", "ISBN-X", 2020, 7));
    assert(b->copies_total == 7);
    assert(b->copies_available == 5);

    /* Decrease total from 7 to 1 — available = max(0, 1-2) = 0 */
    assert(book_update(&lib, id, "Book", "Auth", "ISBN-X", 2020, 1));
    assert(b->copies_total == 1);
    assert(b->copies_available == 0);

    library_free(&lib);
    printf("  PASS: test_update_preserves_issued\n");
}

static void test_validate(void) {
    Book valid = {
        .id = 1,
        .title = "Title",
        .author = "Author",
        .isbn = "",
        .year = 2020,
        .copies_total = 5,
        .copies_available = 3,
        .is_deleted = 0
    };
    assert(book_validate(&valid));

    /* Invalid: id = 0 */
    Book bad_id = valid;
    bad_id.id = 0;
    assert(!book_validate(&bad_id));

    /* Invalid: empty title */
    Book bad_title = valid;
    bad_title.title[0] = '\0';
    assert(!book_validate(&bad_title));

    /* Invalid: negative copies */
    Book bad_copies = valid;
    bad_copies.copies_available = -1;
    assert(!book_validate(&bad_copies));

    /* Invalid: available > total */
    Book bad_avail = valid;
    bad_avail.copies_available = 10;
    assert(!book_validate(&bad_avail));

    printf("  PASS: test_validate\n");
}

static void test_capacity_growth(void) {
    Library lib = make_test_lib();

    /* Add more books than the initial capacity (16) to trigger realloc */
    for (int i = 0; i < 50; i++) {
        char title[64];
        snprintf(title, sizeof(title), "Book %d", i);
        int id = book_add(&lib, title, "Author", "", 2020, 1);
        assert(id > 0);
    }

    assert(lib.book_count == 50);
    assert(lib.book_capacity >= 50);

    /* Verify all books are still accessible */
    for (int i = 1; i <= 50; i++) {
        assert(book_find_by_id(&lib, i) != NULL);
    }

    library_free(&lib);
    printf("  PASS: test_capacity_growth\n");
}

int main(void) {
    printf("Running book unit tests...\n");

    test_add_basic();
    test_add_multiple();
    test_duplicate_isbn_rejected();
    test_empty_isbn_no_conflict();
    test_find_by_id();
    test_find_by_isbn();
    test_search();
    test_soft_delete();
    test_update();
    test_update_preserves_issued();
    test_validate();
    test_capacity_growth();

    printf("All book tests passed!\n");
    return 0;
}
