#include "factorial.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_E
#define M_E 2.71828182845904523536
#endif

FactStatus fact_uint64(int n, uint64_t *out) {
    if (!out) return FACT_ERR_OVERFLOW;
    if (n < 0) return FACT_ERR_NEGATIVE;
    if (n > MAX_FACT_UINT64) return FACT_ERR_OVERFLOW;

    uint64_t res = 1;
    for (int i = 2; i <= n; i++) {
        res *= (uint64_t)i;
    }

    *out = res;
    return FACT_OK;
}

FactStatus fact_big(int n, char *out, size_t out_len) {
    if (!out || out_len == 0) return FACT_ERR_OVERFLOW;
    if (n < 0) return FACT_ERR_NEGATIVE;
    if (n > 1000) return FACT_ERR_OVERFLOW;

    if (n == 0 || n == 1) {
        snprintf(out, out_len, "1");
        return FACT_OK;
    }

    /* Buffer to hold up to 4000 digits */
    unsigned char digits[4096];
    memset(digits, 0, sizeof(digits));
    digits[0] = 1;
    size_t num_digits = 1;

    for (int multiplier = 2; multiplier <= n; multiplier++) {
        unsigned int carry = 0;
        for (size_t i = 0; i < num_digits; i++) {
            unsigned int prod = (unsigned int)digits[i] * (unsigned int)multiplier + carry;
            digits[i] = (unsigned char)(prod % 10);
            carry = prod / 10;
        }
        while (carry > 0) {
            if (num_digits >= sizeof(digits)) {
                return FACT_ERR_OVERFLOW;
            }
            digits[num_digits++] = (unsigned char)(carry % 10);
            carry /= 10;
        }
    }

    if (num_digits >= out_len) {
        return FACT_ERR_OVERFLOW;
    }

    /* Convert reverse-stored digits to string */
    for (size_t i = 0; i < num_digits; i++) {
        out[i] = (char)('0' + digits[num_digits - 1 - i]);
    }
    out[num_digits] = '\0';

    return FACT_OK;
}

FactStatus fact_permutations(int n, int r, uint64_t *out) {
    if (!out) return FACT_ERR_OVERFLOW;
    if (n < 0 || r < 0 || r > n) return FACT_ERR_NEGATIVE;

    uint64_t res = 1;
    for (int i = 0; i < r; i++) {
        uint64_t factor = (uint64_t)(n - i);
        if (factor > 0 && res > UINT64_MAX / factor) {
            return FACT_ERR_OVERFLOW;
        }
        res *= factor;
    }

    *out = res;
    return FACT_OK;
}

FactStatus fact_combinations(int n, int r, uint64_t *out) {
    if (!out) return FACT_ERR_OVERFLOW;
    if (n < 0 || r < 0 || r > n) return FACT_ERR_NEGATIVE;

    /* Symmetry optimization: C(n, r) = C(n, n - r) */
    if (r > n - r) {
        r = n - r;
    }

    uint64_t res = 1;
    for (int i = 1; i <= r; i++) {
        uint64_t factor = (uint64_t)(n - i + 1);
        if (res > UINT64_MAX / factor) {
            return FACT_ERR_OVERFLOW;
        }
        res = (res * factor) / (uint64_t)i;
    }

    *out = res;
    return FACT_OK;
}

double fact_stirling(int n) {
    if (n <= 0) return 1.0;
    double nd = (double)n;
    return sqrt(2.0 * M_PI * nd) * pow(nd / M_E, nd);
}
