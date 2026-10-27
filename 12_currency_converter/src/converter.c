#include "converter.h"
#include "safe_utils.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <math.h>

#ifdef _WIN32
    #include <io.h>
    #define fsync_fd(fd) _commit(fd)
#else
    #include <unistd.h>
    #define fsync_fd(fd) fsync(fd)
#endif

static const Currency DEFAULT_CURRENCIES[8] = {
    {"USD", "US Dollar",         1.000000, 2},
    {"EUR", "Euro",              0.920000, 2},
    {"GBP", "British Pound",     0.790000, 2},
    {"JPY", "Japanese Yen",    152.350000, 0},
    {"CAD", "Canadian Dollar",   1.380000, 2},
    {"AUD", "Australian Dollar", 1.510000, 2},
    {"CHF", "Swiss Franc",       0.860000, 2},
    {"INR", "Indian Rupee",     84.050000, 2}
};

void converter_init(RatesTable *table) {
    if (!table) return;
    memset(table, 0, sizeof(RatesTable));

    strncpy(table->as_of_date, "2026-10-26", sizeof(table->as_of_date) - 1);
    strncpy(table->base_code, "USD", sizeof(table->base_code) - 1);

    table->count = 8;
    for (int i = 0; i < 8; i++) {
        table->currencies[i] = DEFAULT_CURRENCIES[i];
    }

    const char *env = getenv("DATA_DIR");
    if (env && env[0] != '\0') {
        snprintf(table->data_dir, sizeof(table->data_dir), "%s", env);
    } else {
        snprintf(table->data_dir, sizeof(table->data_dir), "./data");
    }
}

int converter_find_currency(const RatesTable *table, const char *code) {
    if (!table || !code) return -1;
    for (int i = 0; i < table->count; i++) {
        if (strcasecmp(table->currencies[i].code, code) == 0) {
            return i;
        }
    }
    return -1;
}

bool converter_load_rates(RatesTable *table) {
    if (!table) return false;
    if (!util_ensure_dir(table->data_dir)) return false;

    char path[512];
    snprintf(path, sizeof(path), "%s/%s", table->data_dir, DEFAULT_RATES_FILE);

    FILE *fp = fopen(path, "r");
    if (!fp) {
        /* File absent — save default table to disk and continue */
        return converter_save_rates(table);
    }

    char line[256];
    int loaded = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        /* Trim whitespace */
        char *start = line;
        while (*start != '\0' && isspace((unsigned char)*start)) start++;
        if (*start == '#' || *start == '\0') continue;

        if (strncmp(start, "DATE=", 5) == 0) {
            char *d = start + 5;
            char *nl = strchr(d, '\n');
            if (nl) *nl = '\0';
            while (*d != '\0' && isspace((unsigned char)*d)) d++;
            strncpy(table->as_of_date, d, sizeof(table->as_of_date) - 1);
            continue;
        }

        if (strncmp(start, "BASE=", 5) == 0) {
            char *b = start + 5;
            char *nl = strchr(b, '\n');
            if (nl) *nl = '\0';
            while (*b != '\0' && isspace((unsigned char)*b)) b++;
            strncpy(table->base_code, b, sizeof(table->base_code) - 1);
            continue;
        }

        /* Format: CODE:Name:Rate:Decimals */
        char code[CURRENCY_CODE_LEN + 4] = {0};
        char name[32] = {0};
        double rate = 0.0;
        int decimals = 2;

        if (sscanf(start, "%3[^:]:%31[^:]:%lf:%d", code, name, &rate, &decimals) >= 3) {
            if (rate > 0.0 && !(rate != rate) && loaded < MAX_CURRENCIES) {
                strncpy(table->currencies[loaded].code, code, sizeof(table->currencies[loaded].code) - 1);
                strncpy(table->currencies[loaded].name, name, sizeof(table->currencies[loaded].name) - 1);
                table->currencies[loaded].rate_to_base = rate;
                table->currencies[loaded].decimals = (decimals >= 0 && decimals <= 4) ? decimals : 2;
                loaded++;
            }
        }
    }

    fclose(fp);

    if (loaded >= 8) {
        table->count = loaded;
    }
    return true;
}

bool converter_save_rates(const RatesTable *table) {
    if (!table) return false;
    if (!util_ensure_dir(table->data_dir)) return false;

    char tmp_path[512];
    char final_path[512];
    snprintf(tmp_path, sizeof(tmp_path), "%s/%s.tmp", table->data_dir, DEFAULT_RATES_FILE);
    snprintf(final_path, sizeof(final_path), "%s/%s", table->data_dir, DEFAULT_RATES_FILE);

    FILE *fp = fopen(tmp_path, "w");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open '%s' for writing rates.\n", tmp_path);
        return false;
    }

    fprintf(fp, "# Currency Rates File (Static Sample Data - Educational Use Only)\n");
    fprintf(fp, "DATE=%s\n", table->as_of_date);
    fprintf(fp, "BASE=%s\n", table->base_code);

    for (int i = 0; i < table->count; i++) {
        const Currency *c = &table->currencies[i];
        fprintf(fp, "%s:%s:%.6f:%d\n", c->code, c->name, c->rate_to_base, c->decimals);
    }

    if (fflush(fp) != 0) {
        fclose(fp);
        return false;
    }

    int fd = fileno(fp);
    if (fd >= 0) {
        (void)fsync_fd(fd);
    }
    fclose(fp);

    if (rename(tmp_path, final_path) != 0) {
        perror("Error renaming temporary rates file");
        return false;
    }

    return true;
}

bool converter_convert(const RatesTable *table, const char *from_code, const char *to_code,
                       double amount, double *out_result, int64_t *out_minor_units) {
    if (!table || !from_code || !to_code) return false;

    /*
     * Critical financial input safety:
     * Reject negative, zero, NaN, +Inf, -Inf using !(amount >= min && amount <= max).
     */
    if (!(amount >= 0.0001 && amount <= 1000000000.0)) {
        return false;
    }

    int from_idx = converter_find_currency(table, from_code);
    int to_idx = converter_find_currency(table, to_code);
    if (from_idx < 0 || to_idx < 0) return false;

    const Currency *c_from = &table->currencies[from_idx];
    const Currency *c_to = &table->currencies[to_idx];

    /* Convert to base (USD), then to target */
    double in_base = amount / c_from->rate_to_base;
    double target_amount = in_base * c_to->rate_to_base;

    /* Calculate integer minor units to eliminate floating-point representation drift */
    double factor = pow(10.0, (double)c_to->decimals);
    int64_t minor = (int64_t)llround(target_amount * factor);

    if (out_minor_units) {
        *out_minor_units = minor;
    }
    if (out_result) {
        *out_result = (double)minor / factor;
    }

    return true;
}

bool converter_update_rate(RatesTable *table, const char *code, double new_rate) {
    if (!table || !code) return false;
    if (!(new_rate >= 0.000001 && new_rate <= 1000000.0)) {
        return false;
    }

    int idx = converter_find_currency(table, code);
    if (idx < 0) return false;

    table->currencies[idx].rate_to_base = new_rate;
    return converter_save_rates(table);
}

void converter_list_rates(const RatesTable *table) {
    if (!table) return;

    printf("\n  %s\n", STATIC_RATES_DISCLAIMER);
    printf("  Exchange Rates (As of: %s | Base: %s)\n", table->as_of_date, table->base_code);
    printf("  +------+-----------------------+---------------+----------------+\n");
    printf("  | Code | Currency Name         | Rate to USD   | Inverse (1/x)  |\n");
    printf("  +------+-----------------------+---------------+----------------+\n");

    for (int i = 0; i < table->count; i++) {
        const Currency *c = &table->currencies[i];
        double inv = (c->rate_to_base > 0.0) ? (1.0 / c->rate_to_base) : 0.0;
        printf("  | %-4s | %-21.21s | %13.6f | %14.6f |\n",
               c->code, c->name, c->rate_to_base, inv);
    }
    printf("  +------+-----------------------+---------------+----------------+\n\n");
}
