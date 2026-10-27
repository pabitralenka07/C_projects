#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include "factorial.h"

static void test_uint64_factorials(void) {
    uint64_t res;
    assert(fact_uint64(0, &res) == FACT_OK && res == 1);
    assert(fact_uint64(1, &res) == FACT_OK && res == 1);
    assert(fact_uint64(5, &res) == FACT_OK && res == 120);
    assert(fact_uint64(10, &res) == FACT_OK && res == 3628800);
    assert(fact_uint64(20, &res) == FACT_OK && res == 2432902008176640000ULL);

    /* Overflow for 21! */
    assert(fact_uint64(21, &res) == FACT_ERR_OVERFLOW);
    /* Negative */
    assert(fact_uint64(-5, &res) == FACT_ERR_NEGATIVE);
    printf("  [PASS] test_uint64_factorials\n");
}

static void test_big_factorials(void) {
    char buf[1024];
    assert(fact_big(0, buf, sizeof(buf)) == FACT_OK && strcmp(buf, "1") == 0);
    assert(fact_big(5, buf, sizeof(buf)) == FACT_OK && strcmp(buf, "120") == 0);
    assert(fact_big(20, buf, sizeof(buf)) == FACT_OK &&
           strcmp(buf, "2432902008176640000") == 0);

    /* 100! has exactly 158 digits and ends with 24 trailing zeros */
    assert(fact_big(100, buf, sizeof(buf)) == FACT_OK);
    size_t len = strlen(buf);
    assert(len == 158);
    /* Check last 24 characters are '0' */
    for (size_t i = len - 24; i < len; i++) {
        assert(buf[i] == '0');
    }
    printf("  [PASS] test_big_factorials\n");
}

static void test_permutations_combinations(void) {
    uint64_t p, c;
    assert(fact_permutations(5, 2, &p) == FACT_OK && p == 20);
    assert(fact_permutations(10, 3, &p) == FACT_OK && p == 720);
    assert(fact_permutations(5, 5, &p) == FACT_OK && p == 120);

    assert(fact_combinations(5, 2, &c) == FACT_OK && c == 10);
    assert(fact_combinations(10, 4, &c) == FACT_OK && c == 210);
    /* Standard 5-card poker hands from 52 cards: 52! / (5! * 47!) = 2,598,960 */
    assert(fact_combinations(52, 5, &c) == FACT_OK && c == 2598960);
    assert(fact_combinations(20, 0, &c) == FACT_OK && c == 1);
    assert(fact_combinations(20, 20, &c) == FACT_OK && c == 1);

    printf("  [PASS] test_permutations_combinations\n");
}

static void test_stirling(void) {
    /* As n grows, relative error drops below 1% */
    uint64_t exact10;
    fact_uint64(10, &exact10);
    double s10 = fact_stirling(10);
    double err10 = fabs(s10 - (double)exact10) / (double)exact10;
    assert(err10 < 0.01); /* Error < 1% at n=10 */

    printf("  [PASS] test_stirling\n");
}

int main(void) {
    printf("=== Running Factorial Calculator Unit Tests ===\n");
    test_uint64_factorials();
    test_big_factorials();
    test_permutations_combinations();
    test_stirling();
    printf("All factorial unit tests passed!\n");
    return 0;
}
