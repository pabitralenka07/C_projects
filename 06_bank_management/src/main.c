/*
 * Bank Management System — Main Entry Point
 * Salted PIN security, deposits, withdrawals, atomic transfers,
 * 3-attempt lockouts, statements, and crash-safe binary persistence.
 */

#include "bank.h"
#include "input.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ADMIN_OVERRIDE_PIN "8888"

static void print_menu(void) {
    printf("\n");
    util_print_header("Banking & Financial Services");
    printf("  1. Open New Bank Account\n");
    printf("  2. Deposit Funds\n");
    printf("  3. Withdraw Cash\n");
    printf("  4. Wire Transfer\n");
    printf("  5. View Account Statement & Balance\n");
    printf("  6. List All Accounts (Admin)\n");
    printf("  7. Unlock Locked Account (Admin Override)\n");
    printf("  8. Exit\n\n");
}

static void do_open_account(Bank *bank) {
    printf("\n--- Open New Account ---\n");
    char name[ACC_NAME_LEN], phone[ACC_PHONE_LEN], pin[16];
    double deposit;

    if (!input_read_string("  Account Holder Full Name: ", name, sizeof(name))) return;
    if (!input_read_string("  Phone Number: ", phone, sizeof(phone))) return;

    for (;;) {
        if (!input_read_string("  Set 4-Digit Security PIN: ", pin, sizeof(pin))) return;
        if (strlen(pin) == 4) break;
        printf("  PIN must be exactly 4 digits. Please try again.\n");
    }

    if (!input_read_double("  Initial Deposit Amount ($): ", &deposit, 0.0, 10000000.0)) return;

    int acc_no = account_create(bank, name, phone, pin, deposit);
    if (acc_no > 0) {
        printf("\n  Congratulations! Account opened successfully.\n");
        printf("  Your Account Number is: #%d\n", acc_no);
        printf("  Initial Balance: $%.2f\n", deposit);
        bank_save(bank);
    }
}

static void do_deposit(Bank *bank) {
    printf("\n--- Deposit Funds ---\n");
    int acc_no;
    if (!input_read_int("  Account Number: ", &acc_no, 1000, 999999)) return;

    Account *acc = account_find_by_id(bank, acc_no);
    if (!acc) {
        printf("  Account #%d not found.\n", acc_no);
        return;
    }

    double amt;
    if (!input_read_double("  Deposit Amount ($): ", &amt, 0.01, 10000000.0)) return;

    if (bank_deposit(bank, acc_no, amt)) {
        printf("\n  Deposit successful! New Balance: $%.2f\n", acc->balance);
        bank_save(bank);
    } else {
        printf("  Deposit failed. Account may be locked.\n");
    }
}

static void do_withdraw(Bank *bank) {
    printf("\n--- Cash Withdrawal ---\n");
    int acc_no;
    char pin[16];
    double amt;

    if (!input_read_int("  Account Number: ", &acc_no, 1000, 999999)) return;

    Account *acc = account_find_by_id(bank, acc_no);
    if (!acc) {
        printf("  Account #%d not found.\n", acc_no);
        return;
    }

    if (acc->is_locked) {
        printf("  Account is LOCKED due to prior failed PIN attempts. Contact Admin.\n");
        return;
    }

    if (!input_read_string("  Security PIN: ", pin, sizeof(pin))) return;
    if (!account_verify_pin(bank, acc_no, pin)) {
        bank_save(bank);
        return;
    }

    if (!input_read_double("  Withdrawal Amount ($): ", &amt, 0.01, 10000000.0)) return;

    if (bank_withdraw(bank, acc_no, amt)) {
        printf("\n  Withdrawal of $%.2f approved! New Balance: $%.2f\n", amt, acc->balance);
        bank_save(bank);
    }
}

static void do_transfer(Bank *bank) {
    printf("\n--- Wire Transfer ---\n");
    int from_id, to_id;
    char pin[16];
    double amt;

    if (!input_read_int("  Your Account Number (Sender): ", &from_id, 1000, 999999)) return;
    Account *from = account_find_by_id(bank, from_id);
    if (!from) {
        printf("  Account #%d not found.\n", from_id);
        return;
    }

    if (from->is_locked) {
        printf("  Your account is LOCKED. Contact Admin.\n");
        return;
    }

    if (!input_read_string("  Sender Security PIN: ", pin, sizeof(pin))) return;
    if (!account_verify_pin(bank, from_id, pin)) {
        bank_save(bank);
        return;
    }

    if (!input_read_int("  Beneficiary Account Number (Recipient): ", &to_id, 1000, 999999)) return;
    Account *to = account_find_by_id(bank, to_id);
    if (!to) {
        printf("  Beneficiary Account #%d not found.\n", to_id);
        return;
    }

    if (!input_read_double("  Transfer Amount ($): ", &amt, 0.01, 10000000.0)) return;

    if (bank_transfer(bank, from_id, to_id, amt)) {
        printf("\n  Transfer successful! Sent $%.2f to %s (#%d).\n", amt, to->name, to_id);
        printf("  Your New Balance: $%.2f\n", from->balance);
        bank_save(bank);
    }
}

static void do_statement(Bank *bank) {
    int acc_no;
    char pin[16];

    if (!input_read_int("  Account Number: ", &acc_no, 1000, 999999)) return;
    Account *acc = account_find_by_id(bank, acc_no);
    if (!acc) {
        printf("  Account #%d not found.\n", acc_no);
        return;
    }

    if (acc->is_locked) {
        printf("  Account is LOCKED. Contact Admin.\n");
        return;
    }

    if (!input_read_string("  Security PIN: ", pin, sizeof(pin))) return;
    if (!account_verify_pin(bank, acc_no, pin)) {
        bank_save(bank);
        return;
    }

    bank_print_statement(bank, acc_no);
}

static void do_admin_unlock(Bank *bank) {
    printf("\n--- Admin Account Unlock ---\n");
    char admin_pin[16];
    if (!input_read_string("  Enter Admin Security Override PIN: ", admin_pin, sizeof(admin_pin))) return;

    if (strcmp(admin_pin, ADMIN_OVERRIDE_PIN) != 0) {
        printf("  Unauthorized access! Invalid admin credentials.\n");
        return;
    }

    int acc_no;
    if (!input_read_int("  Enter Account Number to Unlock: ", &acc_no, 1000, 999999)) return;

    if (account_unlock(bank, acc_no)) {
        printf("\n  Account #%d unlocked successfully! Failed attempts cleared.\n", acc_no);
        bank_save(bank);
    } else {
        printf("  Account #%d not found.\n", acc_no);
    }
}

int main(void) {
    Bank bank;
    bank_init(&bank);

    printf("Loading banking records...\n");
    bank_load(&bank);

    /* Sample initial account seed if starting fresh */
    if (bank.account_count == 0) {
        account_create(&bank, "Alice Smith", "555-0111", "1234", 1500.0);
        account_create(&bank, "Bob Jones", "555-0222", "4321", 850.0);
        bank_save(&bank);
    }
    printf("Ready.\n");

    for (;;) {
        print_menu();
        int choice = input_menu_choice(1, 8);
        if (choice == -1 || choice == 8) break;

        switch (choice) {
        case 1: do_open_account(&bank); break;
        case 2: do_deposit(&bank); break;
        case 3: do_withdraw(&bank); break;
        case 4: do_transfer(&bank); break;
        case 5: do_statement(&bank); break;
        case 6: account_list_all(&bank); break;
        case 7: do_admin_unlock(&bank); break;
        default: break;
        }

        util_press_enter();
    }

    printf("\nSaving banking records...\n");
    bank_save(&bank);
    bank_free(&bank);
    printf("Goodbye!\n");
    return EXIT_SUCCESS;
}
