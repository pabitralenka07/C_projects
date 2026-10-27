#ifndef CALC_H
#define CALC_H

#include <stdbool.h>
#include <stddef.h>

#define CALC_HISTORY_MAX 20

typedef enum {
    CALC_OK = 0,
    CALC_ERR_DIV_ZERO,
    CALC_ERR_SYNTAX,
    CALC_ERR_OVERFLOW,
    CALC_ERR_EMPTY
} CalcStatus;

typedef struct {
    char   expression[128];
    double result;
} CalcHistoryEntry;

typedef struct {
    double           memory;
    bool             has_memory;
    CalcHistoryEntry history[CALC_HISTORY_MAX];
    int              history_count;
} Calculator;

/* Initialize calculator state */
void calc_init(Calculator *calc);

/* Evaluate a standard mathematical expression string (supports +, -, *, /, %, ^, parentheses). */
CalcStatus calc_evaluate(const char *expr, double *result);

/* Basic single-operation calculator */
CalcStatus calc_op(double a, char op, double b, double *result);

/* Convert error enum to human-readable string */
const char *calc_error_str(CalcStatus status);

/* Add calculation to history */
void calc_add_history(Calculator *calc, const char *expr, double result);

/* Clear history */
void calc_clear_history(Calculator *calc);

#endif /* CALC_H */
