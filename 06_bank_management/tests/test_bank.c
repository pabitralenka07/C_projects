#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bank.h"

static void make_temp_dir(char *buf, size_t size) {
    char templ[] = "/tmp/bank_test_XXXXXX";
    char *dir = mkdtemp(templ);
    assert(dir != NULL);
    snprintf(buf, size, "%s", dir);
}

static void cleanup_temp_dir(const char *dir) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", dir);
    (void)system(cmd);
}

static void test_account_creation_and_pin(void) {
    Bank b;
    bank_init(&b);

    int acc1 = account_create(&b, "Charles Babbage", "555-1791", "2468", 500.0);
    assert(acc1 == 1001);
    assert(b.account_count == 1);

    Account *a = account_find_by_id(&b, 1001);
    assert(a != NULL && a->balance == 500.0);
    assert(!a->is_locked);
    assert(strlen(a->salt) == 16);
    assert(strlen(a->pin_hash) == 64);

    /* Verify correct PIN */
    assert(account_verify_pin(&b, 1001, "2468"));

    /* Verify incorrect PIN fails */
    assert(!account_verify_pin(&b, 1001, "0000"));

    bank_free(&b);
    printf("  [PASS] test_account_creation_and_pin\n");
}

static void test_lockout_mechanism(void) {
    Bank b;
    bank_init(&b);

    int acc_no = account_create(&b, "Ada Lovelace", "555-1815", "1234", 1000.0);
    Account *a = account_find_by_id(&b, acc_no);
    assert(!a->is_locked);

    /* 3 consecutive failures */
    assert(!account_verify_pin(&b, acc_no, "9991"));
    assert(!account_verify_pin(&b, acc_no, "9992"));
    assert(!account_verify_pin(&b, acc_no, "9993"));

    /* Now account must be locked */
    assert(a->is_locked);
    assert(!account_verify_pin(&b, acc_no, "1234")); /* Even correct PIN is rejected when locked */
    assert(!bank_withdraw(&b, acc_no, 50.0));

    /* Admin unlock */
    assert(account_unlock(&b, acc_no));
    assert(!a->is_locked);
    assert(account_verify_pin(&b, acc_no, "1234")); /* Works again! */

    bank_free(&b);
    printf("  [PASS] test_lockout_mechanism\n");
}

static void test_deposit_and_withdrawal(void) {
    Bank b;
    bank_init(&b);

    int acc_no = account_create(&b, "Katherine Johnson", "555-1918", "4321", 200.0);
    Account *a = account_find_by_id(&b, acc_no);

    /* Deposit */
    assert(bank_deposit(&b, acc_no, 300.0));
    assert(a->balance == 500.0);

    /* Withdraw valid amount */
    assert(bank_withdraw(&b, acc_no, 150.0));
    assert(a->balance == 350.0);

    /* Withdraw beyond balance (should fail) */
    assert(!bank_withdraw(&b, acc_no, 999.0));
    assert(a->balance == 350.0);

    bank_free(&b);
    printf("  [PASS] test_deposit_and_withdrawal\n");
}

static void test_atomic_transfer(void) {
    Bank b;
    bank_init(&b);

    int a1 = account_create(&b, "Sender", "555-0001", "1111", 1000.0);
    int a2 = account_create(&b, "Receiver", "555-0002", "2222", 200.0);

    /* Transfer $400 from a1 to a2 */
    assert(bank_transfer(&b, a1, a2, 400.0));
    Account *acc1 = account_find_by_id(&b, a1);
    Account *acc2 = account_find_by_id(&b, a2);
    assert(acc1->balance == 600.0);
    assert(acc2->balance == 600.0);

    /* Transfer exceeding balance should fail */
    assert(!bank_transfer(&b, a1, a2, 9999.0));
    assert(acc1->balance == 600.0);

    bank_free(&b);
    printf("  [PASS] test_atomic_transfer\n");
}

static void test_persistence_roundtrip(void) {
    char test_dir[256];
    make_temp_dir(test_dir, sizeof(test_dir));
    setenv("DATA_DIR", test_dir, 1);

    Bank b1;
    bank_init(&b1);
    bank_load(&b1);

    int acc1 = account_create(&b1, "Grace Hopper", "555-1906", "9876", 2500.0);
    bank_deposit(&b1, acc1, 500.0);
    assert(bank_save(&b1));
    bank_free(&b1);

    Bank b2;
    bank_init(&b2);
    assert(bank_load(&b2));
    assert(b2.account_count == 1);
    assert(b2.accounts[0].id == acc1);
    assert(b2.accounts[0].balance == 3000.0);
    assert(strcmp(b2.accounts[0].name, "Grace Hopper") == 0);
    /* Verify PIN still verifies correctly from persisted salt & hash */
    assert(account_verify_pin(&b2, acc1, "9876"));

    bank_free(&b2);
    cleanup_temp_dir(test_dir);
    printf("  [PASS] test_persistence_roundtrip\n");
}

int main(void) {
    printf("=== Running Bank Management Unit Tests ===\n");
    test_account_creation_and_pin();
    test_lockout_mechanism();
    test_deposit_and_withdrawal();
    test_atomic_transfer();
    test_persistence_roundtrip();
    printf("All bank management unit tests passed!\n");
    return 0;
}
