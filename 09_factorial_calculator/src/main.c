/*
 * Factorial Calculator — Main Entry Point
 * Supports 64-bit fast factorial, arbitrary precision big-int factorial,
 * permutations, combinations, and Stirling's approximation.
 */

#include "factorial.h"
#include "input.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>

static void print_menu(void) {
    printf("\n");
    util_print_header("Factorial & Combinatorics Calculator");
    printf("  1. Standard 64-bit Factorial (n <= 20)\n");
    printf("  2. Arbitrary Precision Big-Int Factorial (n <= 1000)\n");
    printf("  3. Permutations P(n, r)\n");
    printf("  4. Combinations C(n, r)\n");
    printf("  5. Stirling's Approximation Comparison\n");
    printf("  6. Exit\n\n");
}

static void do_standard_factorial(void) {
    printf("\n--- Standard 64-bit Factorial ---\n");
    int n;
    if (!input_read_int("  Enter n (0 to 20): ", &n, 0, 20)) return;

    uint64_t result;
    FactStatus st = fact_uint64(n, &result);
    if (st == FACT_OK) {
        printf("\n  %d! = %" PRIu64 "\n", n, result);
    } else {
        printf("  Computation failed.\n");
    }
}

static void do_big_factorial(void) {
    printf("\n--- Arbitrary Precision Big-Int Factorial ---\n");
    int n;
    if (!input_read_int("  Enter n (0 to 1000): ", &n, 0, 1000)) return;

    static char buf[4096];
    FactStatus st = fact_big(n, buf, sizeof(buf));
    if (st == FACT_OK) {
        printf("\n  %d! = %s\n", n, buf);
        printf("  (Total digits: %zu)\n", strlen(buf));
    } else {
        printf("  Computation overflowed internal capacity.\n");
    }
}

static void do_permutations(void) {
    printf("\n--- Permutations P(n, r) ---\n");
    int n, r;
    if (!input_read_int("  Enter total items n (0 to 100): ", &n, 0, 100)) return;
    if (!input_read_int("  Enter items to arrange r (0 to n): ", &r, 0, n)) return;

    uint64_t result;
    FactStatus st = fact_permutations(n, r, &result);
    if (st == FACT_OK) {
        printf("\n  P(%d, %d) = %" PRIu64 "\n", n, r, result);
    } else {
        printf("  Error: Result exceeds 64-bit unsigned integer capacity.\n");
    }
}

static void do_combinations(void) {
    printf("\n--- Combinations C(n, r) ---\n");
    int n, r;
    if (!input_read_int("  Enter total items n (0 to 100): ", &n, 0, 100)) return;
    if (!input_read_int("  Enter items to choose r (0 to n): ", &r, 0, n)) return;

    uint64_t result;
    FactStatus st = fact_combinations(n, r, &result);
    if (st == FACT_OK) {
        printf("\n  C(%d, %d) = %" PRIu64 "\n", n, r, result);
    } else {
        printf("  Error: Result exceeds 64-bit unsigned integer capacity.\n");
    }
}

static void do_stirling(void) {
    printf("\n--- Stirling's Approximation Comparison ---\n");
    int n;
    if (!input_read_int("  Enter n (1 to 20): ", &n, 1, 20)) return;

    uint64_t exact_val;
    fact_uint64(n, &exact_val);
    double stirling_val = fact_stirling(n);
    double exact_double = (double)exact_val;
    double rel_error = fabs(stirling_val - exact_double) / exact_double * 100.0;

    printf("\n  %d! Exact:     %" PRIu64 "\n", n, exact_val);
    printf("  %d! Stirling:  %.6f\n", n, stirling_val);
    printf("  Relative error: %.4f%%\n", rel_error);
}

int main(void) {
    for (;;) {
        print_menu();
        int choice = input_menu_choice(1, 6);
        if (choice == -1 || choice == 6) {
            break;
        }

        switch (choice) {
        case 1:
            do_standard_factorial();
            break;
        case 2:
            do_big_factorial();
            break;
        case 3:
            do_permutations();
            break;
        case 4:
            do_combinations();
            break;
        case 5:
            do_stirling();
            break;
        default:
            break;
        }

        util_press_enter();
    }

    printf("\nThank you for using Factorial Calculator. Goodbye!\n");
    return EXIT_SUCCESS;
}
