#include "converter.h"
#include "safe_utils.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

static void menu_convert(const RatesTable *table) {
    printf("\n--- Currency Conversion ---\n");
    printf("  %s\n", STATIC_RATES_DISCLAIMER);
    printf("  Rates as of: %s\n\n", table->as_of_date);

    char from[16];
    if (!safe_read_string("Enter Source Currency Code (e.g. USD, EUR, GBP): ", from, sizeof(from), false)) return;

    if (converter_find_currency(table, from) < 0) {
        printf("  Error: Currency code '%s' is not supported.\n", from);
        return;
    }

    char to[16];
    if (!safe_read_string("Enter Target Currency Code (e.g. INR, JPY, EUR): ", to, sizeof(to), false)) return;

    if (converter_find_currency(table, to) < 0) {
        printf("  Error: Currency code '%s' is not supported.\n", to);
        return;
    }

    double amount = 0.0;
    if (!safe_read_double("Enter Amount to Convert (> 0): ", &amount, 0.01, 1000000000.0)) {
        return;
    }

    double result = 0.0;
    int64_t minor_units = 0;

    if (converter_convert(table, from, to, amount, &result, &minor_units)) {
        int to_idx = converter_find_currency(table, to);
        int decimals = table->currencies[to_idx].decimals;

        printf("\n===============================================\n");
        printf("  Conversion Result (As of %s):\n", table->as_of_date);
        printf("  %.2f %s = %.*f %s\n", amount, from, decimals, result, to);
        printf("  Exact integer minor units: %lld\n", (long long)minor_units);
        printf("===============================================\n");
    } else {
        printf("\n  [FAILED] Conversion failed. Please check input parameters.\n");
    }
}

static void menu_update_rate(RatesTable *table) {
    printf("\n--- Update Currency Rate ---\n");
    printf("  %s\n\n", STATIC_RATES_DISCLAIMER);

    char code[16];
    if (!safe_read_string("Enter Currency Code to update (e.g. EUR): ", code, sizeof(code), false)) return;

    int idx = converter_find_currency(table, code);
    if (idx < 0) {
        printf("  Currency '%s' not found in table.\n", code);
        return;
    }

    printf("Current rate for %s (%s) relative to USD: %.6f\n",
           table->currencies[idx].code, table->currencies[idx].name, table->currencies[idx].rate_to_base);

    double new_rate = 0.0;
    if (!safe_read_double("Enter New Rate relative to 1.0 USD (> 0): ", &new_rate, 0.000001, 1000000.0)) {
        return;
    }

    if (converter_update_rate(table, code, new_rate)) {
        printf("\n  [SUCCESS] Rate for %s updated to %.6f and saved to rates.txt!\n", code, new_rate);
    } else {
        printf("\n  [FAILED] Could not update rate.\n");
    }
}

int main(void) {
    RatesTable table;
    converter_init(&table);
    converter_load_rates(&table);

    bool running = true;
    while (running) {
        safe_clear_screen();
        printf("===============================================\n");
        printf("             Currency Converter                \n");
        printf("===============================================\n");
        printf("  %s\n", STATIC_RATES_DISCLAIMER);
        printf("  Rates As Of Date: %s\n", table.as_of_date);
        printf("===============================================\n");
        printf("  1. Convert Currency\n");
        printf("  2. View Exchange Rates Table\n");
        printf("  3. Update an Exchange Rate\n");
        printf("  4. Exit\n");
        printf("===============================================\n");

        int choice = safe_menu_choice(1, 4);
        if (choice == -1 || choice == 4) {
            break;
        }

        switch (choice) {
            case 1: menu_convert(&table); break;
            case 2: converter_list_rates(&table); break;
            case 3: menu_update_rate(&table); break;
            default: break;
        }

        util_press_enter();
    }

    printf("\nThank you for using Currency Converter. Goodbye!\n");
    return 0;
}
