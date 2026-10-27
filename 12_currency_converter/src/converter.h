#ifndef CONVERTER_H
#define CONVERTER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_CURRENCIES      16
#define CURRENCY_CODE_LEN   4
#define DEFAULT_RATES_FILE  "rates.txt"

#define STATIC_RATES_DISCLAIMER \
    "[NOTICE: Rates are static sample data for educational use only, not live market rates.]"

typedef struct {
    char   code[CURRENCY_CODE_LEN];     /* e.g., "USD", "EUR" */
    char   name[32];                    /* e.g., "US Dollar" */
    double rate_to_base;                /* Exchange rate relative to base (USD = 1.0) */
    int    decimals;                    /* Minor unit decimals: 2 for cents/paise, 0 for yen */
} Currency;

typedef struct {
    char     as_of_date[16];            /* "YYYY-MM-DD" */
    char     base_code[CURRENCY_CODE_LEN];
    Currency currencies[MAX_CURRENCIES];
    int      count;
    char     data_dir[256];
} RatesTable;

/* Lifecycle */
void converter_init(RatesTable *table);
bool converter_load_rates(RatesTable *table);
bool converter_save_rates(const RatesTable *table);

/* Core Conversion */
int  converter_find_currency(const RatesTable *table, const char *code);
bool converter_convert(const RatesTable *table, const char *from_code, const char *to_code,
                       double amount, double *out_result, int64_t *out_minor_units);

/* Rate Management */
bool converter_update_rate(RatesTable *table, const char *code, double new_rate);
void converter_list_rates(const RatesTable *table);

#endif /* CONVERTER_H */
