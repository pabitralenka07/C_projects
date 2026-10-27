#define _DARWIN_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include "converter.h"

static void test_rate_lookups(void) {
    RatesTable table;
    converter_init(&table);

    assert(table.count >= 8);
    assert(converter_find_currency(&table, "USD") >= 0);
    assert(converter_find_currency(&table, "usd") >= 0); /* Case-insensitive */
    assert(converter_find_currency(&table, "EUR") >= 0);
    assert(converter_find_currency(&table, "GBP") >= 0);
    assert(converter_find_currency(&table, "JPY") >= 0);
    assert(converter_find_currency(&table, "INR") >= 0);
    assert(converter_find_currency(&table, "XYZ") == -1); /* Unknown */

    printf("  [PASS] test_rate_lookups\n");
}

static void test_conversion_math(void) {
    RatesTable table;
    converter_init(&table);

    double result = 0.0;
    int64_t minor = 0;

    /* 100 USD to EUR (at 0.92) = 92.00 EUR */
    assert(converter_convert(&table, "USD", "EUR", 100.0, &result, &minor));
    assert(fabs(result - 92.00) < 0.001);
    assert(minor == 9200); /* 9200 cents */

    /* 100 USD to JPY (at 152.35) = 15235 JPY (0 decimals) */
    assert(converter_convert(&table, "USD", "JPY", 100.0, &result, &minor));
    assert(fabs(result - 15235.0) < 0.001);
    assert(minor == 15235);

    /* Cross-currency: 92 EUR to USD = 100 USD */
    assert(converter_convert(&table, "EUR", "USD", 92.0, &result, &minor));
    assert(fabs(result - 100.00) < 0.001);
    assert(minor == 10000);

    printf("  [PASS] test_conversion_math\n");
}

static void test_invalid_inputs_rejected(void) {
    RatesTable table;
    converter_init(&table);

    double result = 0.0;
    int64_t minor = 0;

    /* Negative amounts */
    assert(!converter_convert(&table, "USD", "EUR", -10.0, &result, &minor));
    assert(!converter_convert(&table, "USD", "EUR", 0.0, &result, &minor));

    /* NaN and Inf */
    double nan_val = 0.0 / 0.0;
    double inf_val = 1.0 / 0.0;
    double neg_inf = -1.0 / 0.0;

    assert(!converter_convert(&table, "USD", "EUR", nan_val, &result, &minor));
    assert(!converter_convert(&table, "USD", "EUR", inf_val, &result, &minor));
    assert(!converter_convert(&table, "USD", "EUR", neg_inf, &result, &minor));

    /* Unknown currencies */
    assert(!converter_convert(&table, "INVALID", "EUR", 100.0, &result, &minor));
    assert(!converter_convert(&table, "USD", "INVALID", 100.0, &result, &minor));

    printf("  [PASS] test_invalid_inputs_rejected\n");
}

static void test_rates_file_io(void) {
    char templ[] = "/tmp/curr_test_XXXXXX";
    char *dir = mkdtemp(templ);
    assert(dir != NULL);
    setenv("DATA_DIR", dir, 1);

    RatesTable table;
    converter_init(&table);

    /* Fresh start creates rates.txt */
    assert(converter_load_rates(&table));
    assert(table.count >= 8);

    /* Update EUR rate */
    assert(converter_update_rate(&table, "EUR", 0.950000));

    /* Reload into new table */
    RatesTable table2;
    converter_init(&table2);
    assert(converter_load_rates(&table2));

    int eur_idx = converter_find_currency(&table2, "EUR");
    assert(eur_idx >= 0);
    assert(fabs(table2.currencies[eur_idx].rate_to_base - 0.950000) < 0.00001);

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", dir);
    (void)system(cmd);

    printf("  [PASS] test_rates_file_io\n");
}

int main(void) {
    printf("=== Running Currency Converter Unit Tests ===\n");
    test_rate_lookups();
    test_conversion_math();
    test_invalid_inputs_rejected();
    test_rates_file_io();
    printf("All currency converter unit tests passed successfully!\n");
    return 0;
}
