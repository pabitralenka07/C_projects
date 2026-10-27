#ifndef ATM_H
#define ATM_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>

#define ATM_CARD_NUM_LEN    20
#define ATM_NAME_LEN        64
#define ATM_NOTE_DENOM      20      /* Dispenses in $20 bills */
#define ATM_DAILY_LIMIT     100000  /* $1000.00 in cents */
#define ATM_MAX_FAILED_PINS 3

#define ATM_MAGIC_CARD      "ATMa"
#define ATM_MAGIC_TXN       "ATMt"
#define ATM_CARDS_FILE      "atm_cards.dat"
#define ATM_TXNS_FILE       "atm_txns.dat"

#define ATM_EDUCATIONAL_DISCLAIMER \
    "[DISCLAIMER: Educational ATM simulation — not production cryptographic banking authentication.]"

typedef struct {
    char    card_number[ATM_CARD_NUM_LEN];  /* 16-digit card number */
    char    holder_name[ATM_NAME_LEN];
    char    salt[17];                       /* 16 hex salt chars */
    char    pin_hash[65];                   /* 64 hex digest chars */
    int64_t balance_cents;                  /* Integer cents: avoid floating-point errors */
    int64_t daily_withdrawn_cents;          /* Total withdrawn today */
    int     failed_attempts;                /* Counter for 3-strike lockout */
    int     is_locked;                      /* 1 = Locked */
    int     is_active;                      /* 1 = Active */
} AtmCard;

typedef struct {
    uint32_t id;
    char     card_number[ATM_CARD_NUM_LEN];
    char     type[16];                      /* "WITHDRAW", "DEPOSIT", "PIN_CHANGE" */
    int64_t  amount_cents;
    int64_t  balance_after_cents;
    time_t   timestamp;
} AtmTxn;

typedef struct {
    AtmCard *cards;
    int      card_count;
    int      card_capacity;
    int      next_card_id;

    AtmTxn  *txns;
    int      txn_count;
    int      txn_capacity;
    int      next_txn_id;

    int64_t  cash_tray_cents;               /* Cash physical reserve in ATM machine */
    char     data_dir[256];
} AtmSystem;

/* Lifecycle */
void atm_system_init(AtmSystem *sys);
void atm_system_free(AtmSystem *sys);
bool atm_system_load(AtmSystem *sys);
bool atm_system_save(AtmSystem *sys);

/* Card Operations */
int  atm_card_register(AtmSystem *sys, const char *card_num, const char *holder_name,
                       const char *pin, int64_t initial_deposit_cents);
AtmCard *atm_find_card(AtmSystem *sys, const char *card_num);
bool atm_verify_pin(AtmSystem *sys, const char *card_num, const char *pin);
bool atm_change_pin(AtmSystem *sys, const char *card_num, const char *old_pin, const char *new_pin);
bool atm_unlock_card(AtmSystem *sys, const char *card_num);

/* ATM Session Transactions */
bool atm_withdraw(AtmSystem *sys, const char *card_num, int64_t amount_cents);
bool atm_deposit(AtmSystem *sys, const char *card_num, int64_t amount_cents);
int  atm_get_mini_statement(const AtmSystem *sys, const char *card_num, AtmTxn *out_txns, int max_txns);

#endif /* ATM_H */
