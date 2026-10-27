#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "calc.h"

#define EPSILON 1e-9

static bool d_equal(double a, double b) {
    return fabs(a - b) < EPSILON;
}

static void test_basic_arithmetic(void) {
    double res;
    assert(calc_op(10, '+', 5, &res) == CALC_OK && d_equal(res, 15.0));
    assert(calc_op(10, '-', 4, &res) == CALC_OK && d_equal(res, 6.0));
    assert(calc_op(7, '*', 8, &res) == CALC_OK && d_equal(res, 56.0));
    assert(calc_op(100, '/', 4, &res) == CALC_OK && d_equal(res, 25.0));
    assert(calc_op(17, '%', 5, &res) == CALC_OK && d_equal(res, 2.0));
    assert(calc_op(2, '^', 4, &res) == CALC_OK && d_equal(res, 16.0));
    printf("  [PASS] test_basic_arithmetic\n");
}

static void test_precedence_and_grouping(void) {
    double res;
    /* Precedence: * before + */
    assert(calc_evaluate("2 + 3 * 4", &res) == CALC_OK && d_equal(res, 14.0));
    /* Parentheses override */
    assert(calc_evaluate("(2 + 3) * 4", &res) == CALC_OK && d_equal(res, 20.0));
    /* Nested parentheses */
    assert(calc_evaluate("((1 + 2) * (3 + 4))", &res) == CALC_OK && d_equal(res, 21.0));
    /* Power before multiply: 2 * 3 ^ 2 = 2 * 9 = 18 */
    assert(calc_evaluate("2 * 3 ^ 2", &res) == CALC_OK && d_equal(res, 18.0));
    /* Right associativity of power: 2 ^ 3 ^ 2 = 2 ^ 9 = 512 */
    assert(calc_evaluate("2 ^ 3 ^ 2", &res) == CALC_OK && d_equal(res, 512.0));
    printf("  [PASS] test_precedence_and_grouping\n");
}

static void test_unary_and_decimals(void) {
    double res;
    assert(calc_evaluate("-5 + 10", &res) == CALC_OK && d_equal(res, 5.0));
    assert(calc_evaluate("2.5 * 4.0", &res) == CALC_OK && d_equal(res, 10.0));
    assert(calc_evaluate("-(3 + 4)", &res) == CALC_OK && d_equal(res, -7.0));
    assert(calc_evaluate("10 / 4", &res) == CALC_OK && d_equal(res, 2.5));
    printf("  [PASS] test_unary_and_decimals\n");
}

static void test_error_conditions(void) {
    double res;
    assert(calc_evaluate("10 / 0", &res) == CALC_ERR_DIV_ZERO);
    assert(calc_evaluate("10 % 0", &res) == CALC_ERR_DIV_ZERO);
    assert(calc_evaluate("(2 + 3", &res) == CALC_ERR_SYNTAX);
    assert(calc_evaluate("2 + * 3", &res) == CALC_ERR_SYNTAX);
    assert(calc_evaluate("", &res) == CALC_ERR_EMPTY);
    assert(calc_evaluate("   ", &res) == CALC_ERR_EMPTY);
    assert(calc_op(10, '/', 0, &res) == CALC_ERR_DIV_ZERO);
    assert(calc_op(10, '?', 5, &res) == CALC_ERR_SYNTAX);
    printf("  [PASS] test_error_conditions\n");
}

static void test_history_and_memory(void) {
    Calculator calc;
    calc_init(&calc);
    assert(!calc.has_memory);
    assert(calc.history_count == 0);

    for (int i = 0; i < 25; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "1 + %d", i);
        calc_add_history(&calc, buf, 1.0 + i);
    }
    assert(calc.history_count == CALC_HISTORY_MAX);
    /* Oldest entries should have shifted: last entry is 1 + 24 = 25 */
    assert(d_equal(calc.history[CALC_HISTORY_MAX - 1].result, 25.0));

    calc_clear_history(&calc);
    assert(calc.history_count == 0);
    printf("  [PASS] test_history_and_memory\n");
}

int main(void) {
    printf("=== Running Simple Calculator Unit Tests ===\n");
    test_basic_arithmetic();
    test_precedence_and_grouping();
    test_unary_and_decimals();
    test_error_conditions();
    test_history_and_memory();
    printf("All calculator unit tests passed!\n");
    return 0;
}
