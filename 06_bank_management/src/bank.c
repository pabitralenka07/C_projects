#include "bank.h"
#include "hash.h"
#include "file_ops.h"
#include "input.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_ACCOUNTS "accounts.dat"
#define FILE_TXNS     "transactions.dat"

void bank_init(Bank *bank) {
    if (!bank) return;
    memset(bank, 0, sizeof(Bank));
    bank->next_account_id = 1001;
    bank->next_txn_id     = 1;

    const char *env = getenv("DATA_DIR");
    if (env && env[0] != '\0') {
        snprintf(bank->data_dir, sizeof(bank->data_dir), "%s", env);
    } else {
        snprintf(bank->data_dir, sizeof(bank->data_dir), "./data");
    }
}

void bank_free(Bank *bank) {
    if (!bank) return;
    free(bank->accounts);
    free(bank->transactions);
    memset(bank, 0, sizeof(Bank));
}

static bool load_records(const char *dir, const char *filename,
                         const char *magic, void **out_data,
                         uint32_t *out_count, uint32_t *out_next_id,
                         size_t record_size) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir, filename);

    void *data = NULL;
    uint32_t count = 0, next_id = 1;
    int res = file_validated_load(path, magic, &data, record_size, &count, &next_id);

    if (res == FILE_LOAD_CORRUPT) {
        printf("  WARNING: %s is corrupt. Restoring from backup...\n", filename);
        if (file_recover_backup(path)) {
            res = file_validated_load(path, magic, &data, record_size, &count, &next_id);
        }
        if (res != FILE_LOAD_OK) {
            data = NULL; count = 0; next_id = 1;
        }
    }

    *out_data    = data;
    *out_count   = count;
    *out_next_id = next_id;
    return true;
}

bool bank_load(Bank *bank) {
    if (!bank) return false;
    if (!util_ensure_dir(bank->data_dir)) return false;

    void *data;
    uint32_t count, next_id;

    /* Accounts */
    load_records(bank->data_dir, FILE_ACCOUNTS, BANK_MAGIC_ACC, &data, &count, &next_id, sizeof(Account));
    bank->accounts        = (Account *)data;
    bank->account_count   = (int)count;
    bank->account_capacity= (int)count;
    bank->next_account_id = (int)next_id > 1000 ? (int)next_id : 1001;

    /* Transactions */
    load_records(bank->data_dir, FILE_TXNS, BANK_MAGIC_TXN, &data, &count, &next_id, sizeof(BankTransaction));
    bank->transactions    = (BankTransaction *)data;
    bank->txn_count       = (int)count;
    bank->txn_capacity    = (int)count;
    bank->next_txn_id     = (int)next_id;

    return true;
}

bool bank_save(Bank *bank) {
    if (!bank) return false;
    char path[512];

    snprintf(path, sizeof(path), "%s/%s", bank->data_dir, FILE_ACCOUNTS);
    file_atomic_save(path, BANK_MAGIC_ACC, bank->accounts, sizeof(Account),
                     (uint32_t)bank->account_count, (uint32_t)bank->next_account_id);

    snprintf(path, sizeof(path), "%s/%s", bank->data_dir, FILE_TXNS);
    file_atomic_save(path, BANK_MAGIC_TXN, bank->transactions, sizeof(BankTransaction),
                     (uint32_t)bank->txn_count, (uint32_t)bank->next_txn_id);

    return true;
}

static void log_transaction(Bank *bank, int account_id, const char *type,
                            double amount, double balance_after, int target_id) {
    if (bank->txn_count >= bank->txn_capacity) {
        int new_cap = (bank->txn_capacity == 0) ? 16 : bank->txn_capacity * 2;
        BankTransaction *grown = realloc(bank->transactions, (size_t)new_cap * sizeof(BankTransaction));
        if (!grown) return;
        bank->transactions = grown;
        bank->txn_capacity = new_cap;
    }

    BankTransaction *t = &bank->transactions[bank->txn_count++];
    t->id = bank->next_txn_id++;
    t->account_id = account_id;
    strncpy(t->type, type, sizeof(t->type) - 1);
    t->amount = amount;
    t->balance_after = balance_after;
    t->target_account_id = target_id;
    t->date = time(NULL);
}

/* ------------------------------------------------------------------ */
/* Account Operations                                                 */
/* ------------------------------------------------------------------ */

int account_create(Bank *bank, const char *name, const char *phone,
                   const char *pin, double initial_deposit) {
    if (!bank || !name || !pin || strlen(pin) < 4) return -1;
    if (initial_deposit < 0.0) return -1;

    if (bank->account_count >= bank->account_capacity) {
        int new_cap = (bank->account_capacity == 0) ? 8 : bank->account_capacity * 2;
        Account *grown = realloc(bank->accounts, (size_t)new_cap * sizeof(Account));
        if (!grown) return -1;
        bank->accounts = grown;
        bank->account_capacity = new_cap;
    }

    Account *acc = &bank->accounts[bank->account_count++];
    memset(acc, 0, sizeof(Account));
    acc->id = bank->next_account_id++;
    strncpy(acc->name, name, sizeof(acc->name) - 1);
    if (phone) strncpy(acc->phone, phone, sizeof(acc->phone) - 1);
    acc->balance = initial_deposit;

    /* Generate salt and hash the PIN */
    hash_generate_salt(acc->salt, sizeof(acc->salt));
    hash_compute(pin, acc->salt, acc->pin_hash, sizeof(acc->pin_hash));

    acc->is_locked = 0;
    acc->failed_attempts = 0;
    acc->is_deleted = 0;

    if (initial_deposit > 0.0) {
        log_transaction(bank, acc->id, "DEPOSIT", initial_deposit, acc->balance, 0);
    }

    return acc->id;
}

Account *account_find_by_id(Bank *bank, int id) {
    if (!bank || id <= 0) return NULL;
    for (int i = 0; i < bank->account_count; i++) {
        if (bank->accounts[i].id == id && !bank->accounts[i].is_deleted) {
            return &bank->accounts[i];
        }
    }
    return NULL;
}

bool account_verify_pin(Bank *bank, int account_id, const char *pin) {
    Account *acc = account_find_by_id(bank, account_id);
    if (!acc || !pin) return false;

    if (acc->is_locked) {
        fprintf(stderr, "Error: Account #%d is LOCKED due to excessive failed attempts.\n", account_id);
        return false;
    }

    if (hash_verify(pin, acc->salt, acc->pin_hash)) {
        acc->failed_attempts = 0;
        return true;
    }

    acc->failed_attempts++;
    int remaining = MAX_FAILED_PIN_ATTEMPTS - acc->failed_attempts;
    if (acc->failed_attempts >= MAX_FAILED_PIN_ATTEMPTS) {
        acc->is_locked = 1;
        fprintf(stderr, "\n  ALERT: Account #%d has been LOCKED! (3 failed attempts)\n", account_id);
    } else {
        fprintf(stderr, "  Invalid PIN. %d attempt(s) remaining before account lockout.\n", remaining);
    }
    return false;
}

bool account_unlock(Bank *bank, int account_id) {
    Account *acc = account_find_by_id(bank, account_id);
    if (!acc) return false;
    acc->is_locked = 0;
    acc->failed_attempts = 0;
    return true;
}

bool account_delete(Bank *bank, int id) {
    Account *acc = account_find_by_id(bank, id);
    if (!acc) return false;
    acc->is_deleted = 1;
    return true;
}

void account_list_all(const Bank *bank) {
    if (!bank) return;
    printf("\n");
    util_print_separator(70);
    printf("  %-8s | %-22s | %-12s | %-10s | %s\n",
           "Acc No.", "Account Holder", "Balance ($)", "Status", "Phone");
    util_print_separator(70);

    int count = 0;
    for (int i = 0; i < bank->account_count; i++) {
        const Account *a = &bank->accounts[i];
        if (a->is_deleted) continue;

        printf("  %-8d | %-22.22s | $%-11.2f | %-10s | %s\n",
               a->id, a->name, a->balance,
               a->is_locked ? "LOCKED 🔒" : "ACTIVE 🟢", a->phone);
        count++;
    }
    util_print_separator(70);
    printf("  Total Accounts: %d\n", count);
}

/* ------------------------------------------------------------------ */
/* Banking Operations                                                 */
/* ------------------------------------------------------------------ */

bool bank_deposit(Bank *bank, int account_id, double amount) {
    if (!bank || amount <= 0.0) return false;
    Account *acc = account_find_by_id(bank, account_id);
    if (!acc || acc->is_locked) return false;

    acc->balance += amount;
    log_transaction(bank, acc->id, "DEPOSIT", amount, acc->balance, 0);
    return true;
}

bool bank_withdraw(Bank *bank, int account_id, double amount) {
    if (!bank || amount <= 0.0) return false;
    Account *acc = account_find_by_id(bank, account_id);
    if (!acc || acc->is_locked) return false;

    if (acc->balance < amount) {
        fprintf(stderr, "Error: Insufficient funds! Current balance: $%.2f\n", acc->balance);
        return false;
    }

    acc->balance -= amount;
    log_transaction(bank, acc->id, "WITHDRAW", amount, acc->balance, 0);
    return true;
}

bool bank_transfer(Bank *bank, int from_id, int to_id, double amount) {
    if (!bank || amount <= 0.0 || from_id == to_id) return false;

    Account *from = account_find_by_id(bank, from_id);
    Account *to   = account_find_by_id(bank, to_id);

    if (!from || !to) return false;
    if (from->is_locked || to->is_locked) return false;

    if (from->balance < amount) {
        fprintf(stderr, "Error: Insufficient funds in source account! Balance: $%.2f\n", from->balance);
        return false;
    }

    from->balance -= amount;
    to->balance   += amount;

    log_transaction(bank, from->id, "TRANSFER_OUT", amount, from->balance, to->id);
    log_transaction(bank, to->id,   "TRANSFER_IN",  amount, to->balance,   from->id);

    return true;
}

void bank_print_statement(const Bank *bank, int account_id) {
    if (!bank) return;
    const Account *acc = NULL;
    for (int i = 0; i < bank->account_count; i++) {
        if (bank->accounts[i].id == account_id) {
            acc = &bank->accounts[i];
            break;
        }
    }
    if (!acc) {
        printf("  Account #%d not found.\n", account_id);
        return;
    }

    printf("\n");
    printf("  +-------------------------------------------------------------+\n");
    printf("  |                    ACCOUNT STATEMENT                        |\n");
    printf("  +-------------------------------------------------------------+\n");
    printf("  | Account Number: #%-10d Status: %-25s |\n",
           acc->id, acc->is_locked ? "LOCKED 🔒" : "ACTIVE 🟢");
    printf("  | Account Holder: %-25s Phone: %-15s |\n", acc->name, acc->phone);
    printf("  | Current Balance: $%-42.2f |\n", acc->balance);
    printf("  +-------------------------------------------------------------+\n");
    printf("  | Date       | Type         | Amount ($)   | Balance After ($)|\n");
    printf("  +------------+--------------+--------------+------------------+\n");

    int count = 0;
    for (int i = 0; i < bank->txn_count; i++) {
        const BankTransaction *t = &bank->transactions[i];
        if (t->account_id == account_id) {
            char dstr[16];
            struct tm *tm_info = localtime(&t->date);
            strftime(dstr, sizeof(dstr), "%Y-%m-%d", tm_info);

            printf("  | %-10s | %-12s | %-12.2f | %-16.2f |\n",
                   dstr, t->type, t->amount, t->balance_after);
            count++;
        }
    }

    if (count == 0) {
        printf("  | No transactions recorded for this account.                  |\n");
    }
    printf("  +-------------------------------------------------------------+\n");
}
