#ifndef FACTORIAL_H
#define FACTORIAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_FACT_UINT64 20  /* 20! is the largest factorial fitting in uint64_t */
#define BIG_INT_CAPACITY 10000 /* Digits capacity for large factorials */

typedef enum {
    FACT_OK = 0,
    FACT_ERR_NEGATIVE,
    FACT_ERR_OVERFLOW
} FactStatus;

/* 64-bit standard factorial (exact for 0 <= n <= 20) */
FactStatus fact_uint64(int n, uint64_t *out);

/* Big-integer exact factorial for n up to 1000.
 * Writes formatted base-10 digits into `out` buffer. */
FactStatus fact_big(int n, char *out, size_t out_len);

/* Permutations P(n, r) = n! / (n - r)! */
FactStatus fact_permutations(int n, int r, uint64_t *out);

/* Combinations C(n, r) = n! / (r! * (n - r)!) */
FactStatus fact_combinations(int n, int r, uint64_t *out);

/* Stirling's Approximation for n! */
double fact_stirling(int n);

#endif /* FACTORIAL_H */
