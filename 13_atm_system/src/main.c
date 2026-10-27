#include "atm.h"
#include "safe_utils.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void session_check_balance(const AtmCard *card) {
    printf("\n===============================================\n");
    printf("              Account Balance                  \n");
    printf("===============================================\n");
    printf("  Card Number:    %s\n", card->card_number);
    printf("  Account Holder: %s\n", card->holder_name);
    printf("  Available:      $%.2f\n", (double)card->balance_cents / 100.0);
    printf("  Daily Limit Remaining: $%.2f\n", (double)(ATM_DAILY_LIMIT - card->daily_withdrawn_cents) / 100.0);
    printf("===============================================\n");
}

static void session_withdraw(AtmSystem *sys, AtmCard *card) {
    printf("\n--- Cash Withdrawal ---\n");
    printf("  Dispenses in $%d note denominations only.\n", ATM_NOTE_DENOM);
    printf("  Current Balance: $%.2f | Daily Remaining: $%.2f\n\n",
           (double)card->balance_cents / 100.0,
           (double)(ATM_DAILY_LIMIT - card->daily_withdrawn_cents) / 100.0);

    double amount = 0.0;
    if (!safe_read_double("Enter amount to withdraw (e.g. 40, 60, 100): ", &amount, 20.0, 1000.0)) {
        return;
    }

    int64_t cents = (int64_t)(amount * 100.0 + 0.5);
    if (atm_withdraw(sys, card->card_number, cents)) {
        int notes = (int)(cents / (ATM_NOTE_DENOM * 100));
        printf("\n===============================================\n");
        printf("  [DISPENSING CASH] Please take your %d x $%d bills ($%.2f).\n",
               notes, ATM_NOTE_DENOM, amount);
        printf("  New Balance: $%.2f\n", (double)card->balance_cents / 100.0);
        printf("===============================================\n");
    } else {
        printf("\n  [FAILED] Withdrawal could not be processed.\n");
    }
}

static void session_deposit(AtmSystem *sys, AtmCard *card) {
    printf("\n--- Cash Deposit ---\n");

    double amount = 0.0;
    if (!safe_read_double("Enter deposit amount ($): ", &amount, 1.0, 10000.0)) {
        return;
    }

    int64_t cents = (int64_t)(amount * 100.0 + 0.5);
    if (atm_deposit(sys, card->card_number, cents)) {
        printf("\n  [SUCCESS] Deposited $%.2f successfully!\n", amount);
        printf("  Updated Balance: $%.2f\n", (double)card->balance_cents / 100.0);
    } else {
        printf("\n  [FAILED] Deposit failed.\n");
    }
}

static void session_mini_statement(const AtmSystem *sys, const AtmCard *card) {
    printf("\n======================================================================\n");
    printf("                    ATM MINI STATEMENT (LAST 5 TRANSACTIONS)           \n");
    printf("======================================================================\n");
    printf("  Card: %s | Holder: %s\n", card->card_number, card->holder_name);
    printf("  +----+------------+------------------+------------------+----------+\n");
    printf("  | ID | Action     | Amount           | Balance After    | Status   |\n");
    printf("  +----+------------+------------------+------------------+----------+\n");

    AtmTxn txns[5];
    int count = atm_get_mini_statement(sys, card->card_number, txns, 5);

    if (count == 0) {
        printf("  | No recent transactions recorded on this card.                    |\n");
    } else {
        for (int i = 0; i < count; i++) {
            printf("  | %-2u | %-10.10s | $%15.2f | $%15.2f | SUCCESS  |\n",
                   txns[i].id, txns[i].type,
                   (double)txns[i].amount_cents / 100.0,
                   (double)txns[i].balance_after_cents / 100.0);
        }
    }
    printf("  +----+------------+------------------+------------------+----------+\n");
    printf("  Current Available Balance: $%.2f\n\n", (double)card->balance_cents / 100.0);
}

static void session_change_pin(AtmSystem *sys, AtmCard *card) {
    printf("\n--- Change Card PIN ---\n");

    char old_pin[16], new_pin[16], confirm_pin[16];
    if (!safe_read_string("Enter Current 4-digit PIN: ", old_pin, sizeof(old_pin), false)) return;
    if (!safe_read_string("Enter New 4-digit PIN: ", new_pin, sizeof(new_pin), false)) return;
    if (!safe_read_string("Confirm New 4-digit PIN: ", confirm_pin, sizeof(confirm_pin), false)) return;

    if (strcmp(new_pin, confirm_pin) != 0) {
        printf("  Error: New PIN confirmation does not match.\n");
        return;
    }

    if (atm_change_pin(sys, card->card_number, old_pin, new_pin)) {
        printf("\n  [SUCCESS] Your PIN has been updated successfully!\n");
    } else {
        printf("\n  [FAILED] Could not update PIN. Verify current PIN and try again.\n");
    }
}

static void run_cardholder_session(AtmSystem *sys, AtmCard *card) {
    bool in_session = true;
    while (in_session) {
        safe_clear_screen();
        printf("===============================================\n");
        printf("            Cardholder Session Menu            \n");
        printf("===============================================\n");
        printf("  Card: %s (%s)\n", card->card_number, card->holder_name);
        printf("-----------------------------------------------\n");
        printf("  1. Check Balance\n");
        printf("  2. Cash Withdrawal ($20 bills only)\n");
        printf("  3. Cash Deposit\n");
        printf("  4. Mini Statement (Last 5 Txns)\n");
        printf("  5. Change PIN\n");
        printf("  6. Return Card & Eject\n");
        printf("===============================================\n");

        int choice = safe_menu_choice(1, 6);
        if (choice == -1 || choice == 6) {
            printf("\n  [CARD RETURNED] Please take your card. Session ended.\n");
            break;
        }

        switch (choice) {
            case 1: session_check_balance(card); break;
            case 2: session_withdraw(sys, card); break;
            case 3: session_deposit(sys, card); break;
            case 4: session_mini_statement(sys, card); break;
            case 5: session_change_pin(sys, card); break;
            default: break;
        }

        util_press_enter();
    }
}

static void menu_insert_card(AtmSystem *sys) {
    printf("\n--- Insert ATM Card ---\n");
    char card_num[ATM_CARD_NUM_LEN];
    if (!safe_read_string("Enter 16-digit Card Number: ", card_num, sizeof(card_num), false)) return;

    AtmCard *card = atm_find_card(sys, card_num);
    if (!card) {
        printf("  Error: Card number not recognized by this terminal.\n");
        return;
    }

    if (card->is_locked) {
        printf("  [SECURITY ALERT] This card is LOCKED due to excessive failed attempts.\n");
        return;
    }

    char pin[16];
    if (!safe_read_string("Enter 4-digit PIN: ", pin, sizeof(pin), false)) return;

    if (atm_verify_pin(sys, card_num, pin)) {
        printf("\n  Authentication successful. Welcome, %s!\n", card->holder_name);
        util_press_enter();
        run_cardholder_session(sys, card);
    }
}

static void menu_register_card(AtmSystem *sys) {
    printf("\n--- Issue / Register New Simulation Card ---\n");
    char card_num[ATM_CARD_NUM_LEN];
    if (!safe_read_string("Enter 16-digit Card Number: ", card_num, sizeof(card_num), false)) return;

    char holder[ATM_NAME_LEN];
    if (!safe_read_string("Enter Cardholder Full Name: ", holder, sizeof(holder), false)) return;

    char pin[16];
    if (!safe_read_string("Set 4-digit PIN: ", pin, sizeof(pin), false)) return;

    double deposit = 0.0;
    if (!safe_read_double("Enter Initial Deposit ($): ", &deposit, 10.0, 100000.0)) return;

    int64_t cents = (int64_t)(deposit * 100.0 + 0.5);
    int res = atm_card_register(sys, card_num, holder, pin, cents);
    if (res > 0) {
        printf("\n  [SUCCESS] Card #%s issued successfully for %s!\n", card_num, holder);
    } else {
        printf("\n  [FAILED] Could not register card (duplicate card number).\n");
    }
}

static void menu_unlock_card(AtmSystem *sys) {
    printf("\n--- Unlock Card (Administrative Simulation) ---\n");
    char card_num[ATM_CARD_NUM_LEN];
    if (!safe_read_string("Enter Card Number to Unlock: ", card_num, sizeof(card_num), false)) return;

    if (atm_unlock_card(sys, card_num)) {
        printf("\n  [SUCCESS] Card #%s has been unlocked.\n", card_num);
    } else {
        printf("\n  [FAILED] Card not found.\n");
    }
}

int main(void) {
    AtmSystem sys;
    atm_system_init(&sys);
    atm_system_load(&sys);

    /* If fresh installation with 0 cards, seed standard test cards */
    if (sys.card_count == 0) {
        atm_card_register(&sys, "1111222233334444", "Alice Smith", "1234", 150000); /* $1500 */
        atm_card_register(&sys, "5555666677778888", "Bob Jones",   "4321",  80000); /* $800 */
    }

    bool running = true;
    while (running) {
        safe_clear_screen();
        printf("===============================================\n");
        printf("             ATM Terminal Simulation           \n");
        printf("===============================================\n");
        printf("  %s\n", ATM_EDUCATIONAL_DISCLAIMER);
        printf("  Machine Cash Tray Reserve: $%.2f\n", (double)sys.cash_tray_cents / 100.0);
        printf("===============================================\n");
        printf("  1. Insert Card & Login\n");
        printf("  2. Register New Card\n");
        printf("  3. Unlock a Locked Card\n");
        printf("  4. Exit\n");
        printf("===============================================\n");

        int choice = safe_menu_choice(1, 4);
        if (choice == -1 || choice == 4) {
            break;
        }

        switch (choice) {
            case 1: menu_insert_card(&sys); break;
            case 2: menu_register_card(&sys); break;
            case 3: menu_unlock_card(&sys); break;
            default: break;
        }

        util_press_enter();
    }

    atm_system_save(&sys);
    atm_system_free(&sys);
    printf("\nATM Terminal shut down. Goodbye!\n");
    return 0;
}
