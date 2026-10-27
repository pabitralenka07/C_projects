#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contact.h"

static void test_format_validation(void) {
    /* Valid phone numbers */
    assert(contact_validate_phone("555-123-4567"));
    assert(contact_validate_phone("+1 555 123 4567"));
    assert(contact_validate_phone("(555) 123-4567"));
    assert(contact_validate_phone("1234567890"));

    /* Invalid phone numbers */
    assert(!contact_validate_phone(""));
    assert(!contact_validate_phone("123")); /* Too short */
    assert(!contact_validate_phone("phone-number")); /* Letters */
    assert(!contact_validate_phone("555-123-4567 ext 100")); /* Ext letters */
    assert(!contact_validate_phone("+")); /* No digits */

    /* Valid emails */
    assert(contact_validate_email("alice@example.com"));
    assert(contact_validate_email("bob.smith@work.org"));
    assert(contact_validate_email("carol_123@sub.domain.co"));

    /* Invalid emails */
    assert(!contact_validate_email(""));
    assert(!contact_validate_email("plainaddress"));
    assert(!contact_validate_email("@missinguser.com"));
    assert(!contact_validate_email("user@.com"));
    assert(!contact_validate_email("user@domain"));
    assert(!contact_validate_email("user@domain@extra.com"));
    assert(!contact_validate_email("user name@domain.com"));

    printf("  [PASS] test_format_validation\n");
}

static void test_crud_and_duplicates(void) {
    ContactBook book;
    contact_book_init(&book);

    /* Add contacts */
    int c1 = contact_add(&book, "Ada Lovelace", "555-000-0001", "ada@computing.org", "London", "Colleagues");
    assert(c1 == 1);
    assert(book.count == 1);

    int c2 = contact_add(&book, "Charles Babbage", "555-000-0002", "charles@computing.org", "London", "Colleagues");
    assert(c2 == 2);
    assert(book.count == 2);

    /* Duplicate phone rejection */
    int dup_phone = contact_add(&book, "Imposter", "555-000-0001", "other@computing.org", "London", "Other");
    assert(dup_phone == -1);

    /* Duplicate normalized phone rejection (different separators, same digits) */
    int dup_norm = contact_add(&book, "Imposter 2", "5550000001", "another@computing.org", "London", "Other");
    assert(dup_norm == -1);

    /* Duplicate email rejection (case-insensitive) */
    int dup_email = contact_add(&book, "Imposter 3", "555-000-0099", "ADA@computing.org", "London", "Other");
    assert(dup_email == -1);

    /* Find by ID */
    Contact *c = contact_find_by_id(&book, c1);
    assert(c != NULL);
    assert(strcmp(c->name, "Ada Lovelace") == 0);

    /* Update contact */
    bool updated = contact_update(&book, c1, "Lady Ada Lovelace", NULL, NULL, "Ockham Park", "Pioneers");
    assert(updated);
    assert(strcmp(c->name, "Lady Ada Lovelace") == 0);
    assert(strcmp(c->address, "Ockham Park") == 0);
    assert(strcmp(c->category, "Pioneers") == 0);
    assert(strcmp(c->phone, "555-000-0001") == 0); /* Preserved */

    /* Soft delete */
    bool deleted = contact_delete(&book, c2);
    assert(deleted);
    assert(contact_find_by_id(&book, c2) == NULL);

    /* Deleting non-existent */
    assert(!contact_delete(&book, 9999));

    contact_book_free(&book);
    printf("  [PASS] test_crud_and_duplicates\n");
}

static void test_search_and_sorting(void) {
    ContactBook book;
    contact_book_init(&book);

    contact_add(&book, "Dennis Ritchie", "555-111-0001", "dennis@bell.org", "NJ", "Work");
    contact_add(&book, "Brian Kernighan", "555-111-0002", "bwk@bell.org", "NJ", "Work");
    contact_add(&book, "Ken Thompson", "555-111-0003", "ken@bell.org", "NJ", "Work");

    /* Search by name substring */
    int results[10];
    int count = contact_search(&book, "ker", results, 10);
    assert(count == 1);
    assert(results[0] == 2);

    /* Search by email */
    count = contact_search(&book, "bell.org", results, 10);
    assert(count == 3);

    /* Sort by Name ASC */
    contact_sort(&book, SORT_NAME_ASC);
    assert(strcmp(book.items[0].name, "Brian Kernighan") == 0);
    assert(strcmp(book.items[1].name, "Dennis Ritchie") == 0);
    assert(strcmp(book.items[2].name, "Ken Thompson") == 0);

    /* Sort by Name DESC */
    contact_sort(&book, SORT_NAME_DESC);
    assert(strcmp(book.items[0].name, "Ken Thompson") == 0);
    assert(strcmp(book.items[1].name, "Dennis Ritchie") == 0);
    assert(strcmp(book.items[2].name, "Brian Kernighan") == 0);

    /* Sort by ID ASC */
    contact_sort(&book, SORT_ID_ASC);
    assert(book.items[0].id == 1);
    assert(book.items[1].id == 2);
    assert(book.items[2].id == 3);

    contact_book_free(&book);
    printf("  [PASS] test_search_and_sorting\n");
}

static void test_capacity_growth(void) {
    ContactBook book;
    contact_book_init(&book);

    for (int i = 0; i < 30; i++) {
        char name[64], phone[20], email[64];
        snprintf(name, sizeof(name), "Person #%02d", i + 1);
        snprintf(phone, sizeof(phone), "555-999-%04d", i + 1);
        snprintf(email, sizeof(email), "person%02d@domain.org", i + 1);
        int id = contact_add(&book, name, phone, email, "Address", "General");
        assert(id == i + 1);
    }

    assert(book.count == 30);
    assert(book.capacity >= 30);

    contact_book_free(&book);
    printf("  [PASS] test_capacity_growth\n");
}

int main(void) {
    printf("=== Running Contact Unit Tests ===\n");
    test_format_validation();
    test_crud_and_duplicates();
    test_search_and_sorting();
    test_capacity_growth();
    printf("All contact unit tests passed successfully!\n");
    return 0;
}
