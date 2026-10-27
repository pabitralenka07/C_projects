/*
 * Simple Calculator — Main Entry Point
 * Supports expression evaluation, step-by-step arithmetic,
 * memory recall, and session history.
 */

#include "calc.h"
#include "input.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_menu(const Calculator *calc) {
    printf("\n");
    util_print_header("Simple Calculator");
    if (calc->has_memory) {
        printf("  [Memory: %.6g]\n", calc->memory);
    }
    printf("  1. Evaluate Expression (e.g. 2 + 3 * (4 - 1))\n");
    printf("  2. Standard Arithmetic (A [op] B)\n");
    printf("  3. Memory Operations (Store / Recall / Clear)\n");
    printf("  4. View Calculation History\n");
    printf("  5. Exit\n\n");
}

static void do_evaluate_expression(Calculator *calc) {
    char expr[128];
    printf("\n--- Evaluate Expression ---\n");
    printf("  Supported: +, -, *, /, %%, ^, (, )\n");
    if (!input_read_string("  Expression: ", expr, sizeof(expr))) return;

    double result = 0.0;
    CalcStatus st = calc_evaluate(expr, &result);
    if (st == CALC_OK) {
        printf("\n  = %.8g\n", result);
        calc_add_history(calc, expr, result);
        if (input_confirm("  Store result in memory?", false)) {
            calc->memory = result;
            calc->has_memory = true;
            printf("  Stored %.8g in memory.\n", result);
        }
    } else {
        printf("  %s\n", calc_error_str(st));
    }
}

static void do_standard_op(Calculator *calc) {
    printf("\n--- Standard Operation ---\n");
    double a, b;
    if (!input_read_double("  First operand: ", &a, -1e12, 1e12)) return;

    char op_str[16];
    char op;
    for (;;) {
        if (!input_read_string("  Operator (+, -, *, /, %, ^): ", op_str, sizeof(op_str))) return;
        op = op_str[0];
        if (op == '+' || op == '-' || op == '*' || op == '/' || op == '%' || op == '^' || op == 'x') {
            break;
        }
        printf("  Invalid operator '%c'. Please choose from +, -, *, /, %%, ^.\n", op);
    }

    if (!input_read_double("  Second operand: ", &b, -1e12, 1e12)) return;

    double result = 0.0;
    CalcStatus st = calc_op(a, op, b, &result);
    if (st == CALC_OK) {
        printf("\n  %.8g %c %.8g = %.8g\n", a, op, b, result);
        char buf[64];
        snprintf(buf, sizeof(buf), "%.8g %c %.8g", a, op, b);
        calc_add_history(calc, buf, result);
        if (input_confirm("  Store result in memory?", false)) {
            calc->memory = result;
            calc->has_memory = true;
            printf("  Stored %.8g in memory.\n", result);
        }
    } else {
        printf("  %s\n", calc_error_str(st));
    }
}

static void do_memory_menu(Calculator *calc) {
    printf("\n--- Memory Operations ---\n");
    printf("  1. Store New Value\n");
    printf("  2. Recall Value\n");
    printf("  3. Clear Memory\n");
    printf("  4. Back\n\n");

    int ch = input_menu_choice(1, 4);
    switch (ch) {
    case 1: {
        double val;
        if (input_read_double("  Enter value to store: ", &val, -1e12, 1e12)) {
            calc->memory = val;
            calc->has_memory = true;
            printf("  Memory set to %.8g\n", val);
        }
        break;
    }
    case 2:
        if (calc->has_memory) {
            printf("  Memory value: %.8g\n", calc->memory);
        } else {
            printf("  Memory is empty.\n");
        }
        break;
    case 3:
        calc->memory = 0.0;
        calc->has_memory = false;
        printf("  Memory cleared.\n");
        break;
    default:
        break;
    }
}

static void do_view_history(const Calculator *calc) {
    printf("\n--- Calculation History ---\n");
    if (calc->history_count == 0) {
        printf("  No calculations in history yet.\n");
        return;
    }

    for (int i = 0; i < calc->history_count; i++) {
        printf("  [%2d] %-30s = %.8g\n",
               i + 1,
               calc->history[i].expression,
               calc->history[i].result);
    }
}

int main(void) {
    Calculator calc;
    calc_init(&calc);

    for (;;) {
        print_menu(&calc);
        int choice = input_menu_choice(1, 5);
        if (choice == -1 || choice == 5) {
            break;
        }

        switch (choice) {
        case 1:
            do_evaluate_expression(&calc);
            break;
        case 2:
            do_standard_op(&calc);
            break;
        case 3:
            do_memory_menu(&calc);
            break;
        case 4:
            do_view_history(&calc);
            break;
        default:
            break;
        }

        util_press_enter();
    }

    printf("\nThank you for using Simple Calculator. Goodbye!\n");
    return EXIT_SUCCESS;
}
