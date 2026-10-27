#ifndef BANK_H
#define BANK_H

#include <stdbool.h>
#include <time.h>
#include <stdint.h>
#include <stddef.h>

#define ACC_NAME_LEN    64
#define ACC_PHONE_LEN   16
#define TXN_TYPE_LEN    16

#define BANK_MAGIC_ACC  "BNKa"
#define BANK_MAGIC_TXN  "BNKt"

#define MAX_FAILED_PIN_ATTEMPTS 3

typedef struct {
    int    id;                      /* Account Number (e.g. 1001) */
    char   name[ACC_NAME_LEN];
    char   phone[ACC_PHONE_LEN];
    double balance;
    char   salt[17];                /* 16 hex chars + NUL */
    char   pin_hash[65];            /* 64 hex chars + NUL */
    int    is_locked;               /* 1 = Locked after 3 failed attempts */
    int    failed_attempts;
    int    is_deleted;
} Account;

typedef struct {
    int    id;
    int    account_id;
    char   type[TXN_TYPE_LEN];      /* "DEPOSIT", "WITHDRAW", "TRANSFER" */
    double amount;
    double balance_after;
    int    target_account_id;       /* Target account for transfers, 0 otherwise */
    time_t date;
} BankTransaction;

typedef struct {
    Account         *accounts;
    int              account_count;
    int              account_capacity;
    int              next_account_id;

    BankTransaction *transactions;
    int              txn_count;
    int              txn_capacity;
    int              next_txn_id;

    char             data_dir[256];
} Bank;

/* Lifecycle */
void bank_init(Bank *bank);
void bank_free(Bank *bank);
bool bank_load(Bank *bank);
bool bank_save(Bank *bank);

/* Account Operations */
int      account_create(Bank *bank, const char *name, const char *phone,
                        const char *pin, double initial_deposit);
Account *account_find_by_id(Bank *bank, int id);
bool     account_verify_pin(Bank *bank, int account_id, const char *pin);
bool     account_unlock(Bank *bank, int account_id);
bool     account_delete(Bank *bank, int id);
void     account_list_all(const Bank *bank);

/* Banking Operations */
bool     bank_deposit(Bank *bank, int account_id, double amount);
bool     bank_withdraw(Bank *bank, int account_id, double amount);
bool     bank_transfer(Bank *bank, int from_account_id, int to_account_id, double amount);
void     bank_print_statement(const Bank *bank, int account_id);

#endif /* BANK_H */
