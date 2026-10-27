#include "atm.h"
#include "hash.h"
#include "safe_utils.h"
#include "file_ops.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CARD_CAP 16
#define INITIAL_TXN_CAP  32

void atm_system_init(AtmSystem *sys) {
    if (!sys) return;
    memset(sys, 0, sizeof(AtmSystem));

    sys->cash_tray_cents = 5000000; /* $50,000 initial cash reserve in ATM tray */
    sys->next_card_id = 1;
    sys->next_txn_id = 1;

    const char *env = getenv("DATA_DIR");
    if (env && env[0] != '\0') {
        snprintf(sys->data_dir, sizeof(sys->data_dir), "%s", env);
    } else {
        snprintf(sys->data_dir, sizeof(sys->data_dir), "./data");
    }
}

void atm_system_free(AtmSystem *sys) {
    if (!sys) return;
    free(sys->cards);
    free(sys->txns);
    memset(sys, 0, sizeof(AtmSystem));
}

bool atm_system_load(AtmSystem *sys) {
    if (!sys) return false;
    if (!util_ensure_dir(sys->data_dir)) return false;

    char card_path[512], txn_path[512];
    snprintf(card_path, sizeof(card_path), "%s/%s", sys->data_dir, ATM_CARDS_FILE);
    snprintf(txn_path, sizeof(txn_path), "%s/%s", sys->data_dir, ATM_TXNS_FILE);

    /* 1. Load Cards */
    void *card_data = NULL;
    uint32_t card_count = 0, next_card = 1;
    int res = file_validated_load(card_path, ATM_MAGIC_CARD, &card_data, sizeof(AtmCard),
                                  &card_count, &next_card);

    if (res == FILE_LOAD_CORRUPT) {
        printf("\n  WARNING: ATM cards data file is corrupted!\n");
        if (safe_confirm("  Attempt recovery from backup?")) {
            if (safe_recover_backup(card_path, ATM_MAGIC_CARD, sizeof(AtmCard))) {
                res = file_validated_load(card_path, ATM_MAGIC_CARD, &card_data, sizeof(AtmCard),
                                          &card_count, &next_card);
            }
        }
    }
    if (res == FILE_LOAD_OK && card_data) {
        sys->cards = (AtmCard *)card_data;
        sys->card_count = (int)card_count;
        sys->card_capacity = (int)card_count;
        sys->next_card_id = (int)next_card;
    }

    /* 2. Load Transactions */
    void *txn_data = NULL;
    uint32_t txn_count = 0, next_txn = 1;
    int res_txn = file_validated_load(txn_path, ATM_MAGIC_TXN, &txn_data, sizeof(AtmTxn),
                                      &txn_count, &next_txn);

    if (res_txn == FILE_LOAD_CORRUPT) {
        printf("\n  WARNING: ATM transactions data file is corrupted!\n");
        if (safe_confirm("  Attempt recovery from backup?")) {
            if (safe_recover_backup(txn_path, ATM_MAGIC_TXN, sizeof(AtmTxn))) {
                res_txn = file_validated_load(txn_path, ATM_MAGIC_TXN, &txn_data, sizeof(AtmTxn),
                                              &txn_count, &next_txn);
            }
        }
    }
    if (res_txn == FILE_LOAD_OK && txn_data) {
        sys->txns = (AtmTxn *)txn_data;
        sys->txn_count = (int)txn_count;
        sys->txn_capacity = (int)txn_count;
        sys->next_txn_id = (int)next_txn;
    }

    return true;
}

bool atm_system_save(AtmSystem *sys) {
    if (!sys) return false;
    if (!util_ensure_dir(sys->data_dir)) return false;

    char card_path[512], txn_path[512];
    snprintf(card_path, sizeof(card_path), "%s/%s", sys->data_dir, ATM_CARDS_FILE);
    snprintf(txn_path, sizeof(txn_path), "%s/%s", sys->data_dir, ATM_TXNS_FILE);

    bool ok_cards = safe_atomic_save(card_path, ATM_MAGIC_CARD, sys->cards, sizeof(AtmCard),
                                     (uint32_t)sys->card_count, (uint32_t)sys->next_card_id);
    bool ok_txns = safe_atomic_save(txn_path, ATM_MAGIC_TXN, sys->txns, sizeof(AtmTxn),
                                    (uint32_t)sys->txn_count, (uint32_t)sys->next_txn_id);

    return (ok_cards && ok_txns);
}

AtmCard *atm_find_card(AtmSystem *sys, const char *card_num) {
    if (!sys || !card_num) return NULL;
    for (int i = 0; i < sys->card_count; i++) {
        if (sys->cards[i].is_active && strcmp(sys->cards[i].card_number, card_num) == 0) {
            return &sys->cards[i];
        }
    }
    return NULL;
}

int atm_card_register(AtmSystem *sys, const char *card_num, const char *holder_name,
                       const char *pin, int64_t initial_deposit_cents) {
    if (!sys || !card_num || !holder_name || !pin) return -1;
    if (atm_find_card(sys, card_num) != NULL) {
        return -1; /* Duplicate card */
    }

    if (sys->card_count >= sys->card_capacity) {
        int new_cap = (sys->card_capacity == 0) ? INITIAL_CARD_CAP : sys->card_capacity * 2;
        AtmCard *new_cards = (AtmCard *)realloc(sys->cards, (size_t)new_cap * sizeof(AtmCard));
        if (!new_cards) return -1;
        sys->cards = new_cards;
        sys->card_capacity = new_cap;
    }

    AtmCard *c = &sys->cards[sys->card_count];
    memset(c, 0, sizeof(AtmCard));

    strncpy(c->card_number, card_num, sizeof(c->card_number) - 1);
    strncpy(c->holder_name, holder_name, sizeof(c->holder_name) - 1);
    c->balance_cents = initial_deposit_cents;
    c->daily_withdrawn_cents = 0;
    c->failed_attempts = 0;
    c->is_locked = 0;
    c->is_active = 1;

    /* Compute salted PIN hash */
    hash_generate_salt(c->salt, sizeof(c->salt));
    hash_compute(pin, c->salt, c->pin_hash, sizeof(c->pin_hash));

    sys->card_count++;
    sys->next_card_id++;
    atm_system_save(sys);
    return sys->card_count;
}

bool atm_verify_pin(AtmSystem *sys, const char *card_num, const char *pin) {
    AtmCard *card = atm_find_card(sys, card_num);
    if (!card) return false;

    if (card->is_locked) {
        printf("  [ALERT] This card is LOCKED due to excessive failed attempts.\n");
        return false;
    }

    if (hash_verify(pin, card->salt, card->pin_hash)) {
        card->failed_attempts = 0;
        return true;
    }

    card->failed_attempts++;
    int remaining = ATM_MAX_FAILED_PINS - card->failed_attempts;
    if (remaining > 0) {
        printf("  [ERROR] Invalid PIN! %d attempt(s) remaining before card lockout.\n", remaining);
    } else {
        card->is_locked = 1;
        printf("\n  [SECURITY LOCKOUT] Card #%s has been LOCKED! (3 failed attempts)\n", card_num);
    }

    atm_system_save(sys);
    return false;
}

static bool record_txn(AtmSystem *sys, const char *card_num, const char *type,
                       int64_t amount_cents, int64_t balance_after_cents) {
    if (sys->txn_count >= sys->txn_capacity) {
        int new_cap = (sys->txn_capacity == 0) ? INITIAL_TXN_CAP : sys->txn_capacity * 2;
        AtmTxn *new_txns = (AtmTxn *)realloc(sys->txns, (size_t)new_cap * sizeof(AtmTxn));
        if (!new_txns) return false;
        sys->txns = new_txns;
        sys->txn_capacity = new_cap;
    }

    AtmTxn *t = &sys->txns[sys->txn_count];
    memset(t, 0, sizeof(AtmTxn));

    t->id = (uint32_t)sys->next_txn_id++;
    strncpy(t->card_number, card_num, sizeof(t->card_number) - 1);
    strncpy(t->type, type, sizeof(t->type) - 1);
    t->amount_cents = amount_cents;
    t->balance_after_cents = balance_after_cents;
    t->timestamp = time(NULL);

    sys->txn_count++;
    return true;
}

bool atm_withdraw(AtmSystem *sys, const char *card_num, int64_t amount_cents) {
    AtmCard *card = atm_find_card(sys, card_num);
    if (!card || card->is_locked) return false;

    if (amount_cents <= 0) {
        printf("  Error: Amount must be greater than zero.\n");
        return false;
    }

    int64_t note_cents = (int64_t)ATM_NOTE_DENOM * 100;
    if (amount_cents % note_cents != 0) {
        printf("  Error: ATM dispenses in $%d bills only. Amount must be a multiple of $%d.\n",
               ATM_NOTE_DENOM, ATM_NOTE_DENOM);
        return false;
    }

    if (card->daily_withdrawn_cents + amount_cents > ATM_DAILY_LIMIT) {
        printf("  Error: Exceeds daily withdrawal limit of $%.2f (Remaining limit today: $%.2f).\n",
               (double)ATM_DAILY_LIMIT / 100.0,
               (double)(ATM_DAILY_LIMIT - card->daily_withdrawn_cents) / 100.0);
        return false;
    }

    if (sys->cash_tray_cents < amount_cents) {
        printf("  Error: ATM cash tray has insufficient cash reserves ($%.2f available).\n",
               (double)sys->cash_tray_cents / 100.0);
        return false;
    }

    if (card->balance_cents < amount_cents) {
        printf("  Error: Insufficient account funds. Current balance: $%.2f\n",
               (double)card->balance_cents / 100.0);
        return false;
    }

    card->balance_cents -= amount_cents;
    card->daily_withdrawn_cents += amount_cents;
    sys->cash_tray_cents -= amount_cents;

    record_txn(sys, card_num, "WITHDRAW", amount_cents, card->balance_cents);
    atm_system_save(sys);
    return true;
}

bool atm_deposit(AtmSystem *sys, const char *card_num, int64_t amount_cents) {
    AtmCard *card = atm_find_card(sys, card_num);
    if (!card || card->is_locked) return false;

    if (amount_cents <= 0) {
        printf("  Error: Deposit amount must be positive.\n");
        return false;
    }

    card->balance_cents += amount_cents;
    sys->cash_tray_cents += amount_cents;

    record_txn(sys, card_num, "DEPOSIT", amount_cents, card->balance_cents);
    atm_system_save(sys);
    return true;
}

bool atm_change_pin(AtmSystem *sys, const char *card_num, const char *old_pin, const char *new_pin) {
    AtmCard *card = atm_find_card(sys, card_num);
    if (!card || card->is_locked) return false;

    if (!hash_verify(old_pin, card->salt, card->pin_hash)) {
        printf("  Error: Existing PIN does not match.\n");
        return false;
    }

    hash_generate_salt(card->salt, sizeof(card->salt));
    hash_compute(new_pin, card->salt, card->pin_hash, sizeof(card->pin_hash));

    record_txn(sys, card_num, "PIN_CHANGE", 0, card->balance_cents);
    atm_system_save(sys);
    return true;
}

bool atm_unlock_card(AtmSystem *sys, const char *card_num) {
    AtmCard *card = atm_find_card(sys, card_num);
    if (!card) return false;
    card->is_locked = 0;
    card->failed_attempts = 0;
    return atm_system_save(sys);
}

int atm_get_mini_statement(const AtmSystem *sys, const char *card_num, AtmTxn *out_txns, int max_txns) {
    if (!sys || !card_num || !out_txns || max_txns <= 0) return 0;

    int matched = 0;
    for (int i = sys->txn_count - 1; i >= 0 && matched < max_txns; i--) {
        if (strcmp(sys->txns[i].card_number, card_num) == 0) {
            out_txns[matched++] = sys->txns[i];
        }
    }
    return matched;
}
