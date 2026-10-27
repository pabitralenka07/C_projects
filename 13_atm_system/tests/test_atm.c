#define _DARWIN_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "atm.h"
#include "safe_utils.h"
#include "file_ops.h"

static void test_card_registration_and_pin_verify(void) {
    AtmSystem sys;
    atm_system_init(&sys);

    int id = atm_card_register(&sys, "1234123412341234", "Charlie Brown", "9876", 50000); /* $500 */
    assert(id == 1);
    assert(sys.card_count == 1);

    /* Duplicate card rejected */
    assert(atm_card_register(&sys, "1234123412341234", "Imposter", "0000", 10000) == -1);

    /* Correct PIN */
    assert(atm_verify_pin(&sys, "1234123412341234", "9876"));

    /* Incorrect PIN */
    assert(!atm_verify_pin(&sys, "1234123412341234", "0000"));

    atm_system_free(&sys);
    printf("  [PASS] test_card_registration_and_pin_verify\n");
}

static void test_card_lockout_after_3_attempts(void) {
    AtmSystem sys;
    atm_system_init(&sys);

    atm_card_register(&sys, "9999888877776666", "Lockout User", "1111", 20000);
    AtmCard *card = atm_find_card(&sys, "9999888877776666");
    assert(card != NULL);
    assert(card->failed_attempts == 0);
    assert(card->is_locked == 0);

    /* Attempt 1 */
    assert(!atm_verify_pin(&sys, "9999888877776666", "0001"));
    assert(card->failed_attempts == 1);
    assert(!card->is_locked);

    /* Attempt 2 */
    assert(!atm_verify_pin(&sys, "9999888877776666", "0002"));
    assert(card->failed_attempts == 2);
    assert(!card->is_locked);

    /* Attempt 3 -> Should lock */
    assert(!atm_verify_pin(&sys, "9999888877776666", "0003"));
    assert(card->failed_attempts == 3);
    assert(card->is_locked == 1);

    /* Subsequent verification with correct PIN must still fail due to lock */
    assert(!atm_verify_pin(&sys, "9999888877776666", "1111"));

    /* Admin unlock */
    assert(atm_unlock_card(&sys, "9999888877776666"));
    assert(!card->is_locked);
    assert(card->failed_attempts == 0);
    assert(atm_verify_pin(&sys, "9999888877776666", "1111"));

    atm_system_free(&sys);
    printf("  [PASS] test_card_lockout_after_3_attempts\n");
}

static void test_withdraw_denominations_and_limits(void) {
    AtmSystem sys;
    atm_system_init(&sys);

    atm_card_register(&sys, "3333444455556666", "Dave Miller", "5555", 200000); /* $2000 */

    /* Reject non-multiples of $20 note denomination */
    assert(!atm_withdraw(&sys, "3333444455556666", 2500)); /* $25 */
    assert(!atm_withdraw(&sys, "3333444455556666", 5000)); /* $50 */
    assert(!atm_withdraw(&sys, "3333444455556666", -2000)); /* Negative */

    /* Valid withdrawal: $60 (3 x $20 notes) */
    assert(atm_withdraw(&sys, "3333444455556666", 6000));
    AtmCard *card = atm_find_card(&sys, "3333444455556666");
    assert(card->balance_cents == 194000); /* $1940 remaining */
    assert(card->daily_withdrawn_cents == 6000);

    /* Exceeding daily limit ($1000 limit; already took $60; trying to take $960 is ok, $980 is over) */
    assert(!atm_withdraw(&sys, "3333444455556666", 96000)); /* 6000 + 96000 = 102000 > 100000 */
    assert(atm_withdraw(&sys, "3333444455556666", 94000));  /* 6000 + 94000 = 100000 (Exactly $1000) */
    assert(card->daily_withdrawn_cents == 100000);

    /* Further withdrawal today must fail */
    assert(!atm_withdraw(&sys, "3333444455556666", 2000));

    atm_system_free(&sys);
    printf("  [PASS] test_withdraw_denominations_and_limits\n");
}

static void test_deposit_and_mini_statement(void) {
    AtmSystem sys;
    atm_system_init(&sys);

    atm_card_register(&sys, "7777888899990000", "Eve White", "7777", 10000); /* $100 */

    /* Deposit $50 */
    assert(atm_deposit(&sys, "7777888899990000", 5000));
    AtmCard *card = atm_find_card(&sys, "7777888899990000");
    assert(card->balance_cents == 15000);

    /* Withdraw $40 */
    assert(atm_withdraw(&sys, "7777888899990000", 4000));
    assert(card->balance_cents == 11000);

    /* Mini statement */
    AtmTxn txns[5];
    int count = atm_get_mini_statement(&sys, "7777888899990000", txns, 5);
    assert(count == 2);
    /* Latest is index 0: WITHDRAW */
    assert(strcmp(txns[0].type, "WITHDRAW") == 0);
    assert(txns[0].amount_cents == 4000);
    assert(txns[0].balance_after_cents == 11000);

    /* Previous is index 1: DEPOSIT */
    assert(strcmp(txns[1].type, "DEPOSIT") == 0);
    assert(txns[1].amount_cents == 5000);
    assert(txns[1].balance_after_cents == 15000);

    atm_system_free(&sys);
    printf("  [PASS] test_deposit_and_mini_statement\n");
}

static void test_pin_change(void) {
    AtmSystem sys;
    atm_system_init(&sys);

    atm_card_register(&sys, "1111000011110000", "Grace", "4444", 50000);

    /* Wrong old PIN fails */
    assert(!atm_change_pin(&sys, "1111000011110000", "9999", "8888"));

    /* Correct old PIN succeeds */
    assert(atm_change_pin(&sys, "1111000011110000", "4444", "8888"));

    /* Old PIN no longer works */
    assert(!atm_verify_pin(&sys, "1111000011110000", "4444"));

    /* New PIN works */
    assert(atm_verify_pin(&sys, "1111000011110000", "8888"));

    atm_system_free(&sys);
    printf("  [PASS] test_pin_change\n");
}

static void test_persistence_and_file_integrity(void) {
    char templ[] = "/tmp/atm_test_XXXXXX";
    char *dir = mkdtemp(templ);
    assert(dir != NULL);
    setenv("DATA_DIR", dir, 1);

    AtmSystem s1;
    atm_system_init(&s1);
    assert(atm_system_load(&s1));

    atm_card_register(&s1, "1234567812345678", "Saved User", "1212", 100000);
    assert(atm_system_save(&s1));
    atm_system_free(&s1);

    /* Reload into s2 */
    AtmSystem s2;
    atm_system_init(&s2);
    assert(atm_system_load(&s2));
    assert(s2.card_count == 1);
    AtmCard *c = atm_find_card(&s2, "1234567812345678");
    assert(c != NULL);
    assert(strcmp(c->holder_name, "Saved User") == 0);
    assert(c->balance_cents == 100000);

    /* Deposit creates v2 and rotates v1 to .bak */
    assert(atm_deposit(&s2, "1234567812345678", 20000));

    char card_path[512];
    snprintf(card_path, sizeof(card_path), "%s/%s", dir, ATM_CARDS_FILE);

    /* Corrupt file */
    FILE *fp = fopen(card_path, "wb");
    assert(fp != NULL);
    fputs("CORRUPT_ATM_HEADER", fp);
    fclose(fp);

    /* Recover from backup */
    assert(safe_recover_backup(card_path, ATM_MAGIC_CARD, sizeof(AtmCard)));

    AtmSystem s3;
    atm_system_init(&s3);
    assert(atm_system_load(&s3));
    AtmCard *rec = atm_find_card(&s3, "1234567812345678");
    assert(rec != NULL);
    assert(rec->balance_cents == 100000); /* Restored to v1 */

    atm_system_free(&s2);
    atm_system_free(&s3);

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", dir);
    (void)system(cmd);

    printf("  [PASS] test_persistence_and_file_integrity\n");
}

int main(void) {
    printf("=== Running ATM System Unit Tests ===\n");
    test_card_registration_and_pin_verify();
    test_card_lockout_after_3_attempts();
    test_withdraw_denominations_and_limits();
    test_deposit_and_mini_statement();
    test_pin_change();
    test_persistence_and_file_integrity();
    printf("All ATM system unit tests passed successfully!\n");
    return 0;
}
