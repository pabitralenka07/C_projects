#include "calc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

void calc_init(Calculator *calc) {
    if (!calc) return;
    calc->memory = 0.0;
    calc->has_memory = false;
    calc->history_count = 0;
}

const char *calc_error_str(CalcStatus status) {
    switch (status) {
    case CALC_OK:           return "Success";
    case CALC_ERR_DIV_ZERO: return "Error: Division or modulo by zero";
    case CALC_ERR_SYNTAX:   return "Error: Syntax error or unmatched parenthesis";
    case CALC_ERR_OVERFLOW: return "Error: Numeric overflow or invalid domain";
    case CALC_ERR_EMPTY:    return "Error: Empty expression";
    default:                return "Error: Unknown error";
    }
}

CalcStatus calc_op(double a, char op, double b, double *result) {
    if (!result) return CALC_ERR_SYNTAX;

    switch (op) {
    case '+':
        *result = a + b;
        return CALC_OK;
    case '-':
        *result = a - b;
        return CALC_OK;
    case '*':
    case 'x':
    case 'X':
        *result = a * b;
        return CALC_OK;
    case '/':
        if (fabs(b) < 1e-12) return CALC_ERR_DIV_ZERO;
        *result = a / b;
        return CALC_OK;
    case '%':
        if (fabs(b) < 1e-12) return CALC_ERR_DIV_ZERO;
        *result = fmod(a, b);
        return CALC_OK;
    case '^':
        if (a < 0.0 && floor(b) != b) return CALC_ERR_OVERFLOW;
        *result = pow(a, b);
        if (isnan(*result) || isinf(*result)) return CALC_ERR_OVERFLOW;
        return CALC_OK;
    default:
        return CALC_ERR_SYNTAX;
    }
}

/* ------------------------------------------------------------------ */
/*  Recursive Descent Expression Parser                                */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *src;
    size_t      pos;
    CalcStatus  error;
} Parser;

static void skip_whitespace(Parser *p) {
    while (p->src[p->pos] != '\0' && isspace((unsigned char)p->src[p->pos])) {
        p->pos++;
    }
}

static char peek(Parser *p) {
    skip_whitespace(p);
    return p->src[p->pos];
}

static char get(Parser *p) {
    skip_whitespace(p);
    char c = p->src[p->pos];
    if (c != '\0') p->pos++;
    return c;
}

static double parse_expr(Parser *p);
static double parse_term(Parser *p);
static double parse_power(Parser *p);
static double parse_factor(Parser *p);

static double parse_factor(Parser *p) {
    skip_whitespace(p);
    char c = peek(p);

    /* Unary plus / minus */
    if (c == '+') {
        get(p);
        return parse_factor(p);
    }
    if (c == '-') {
        get(p);
        return -parse_factor(p);
    }

    /* Sub-expression in parentheses */
    if (c == '(') {
        get(p);
        double val = parse_expr(p);
        if (p->error != CALC_OK) return 0.0;
        if (peek(p) != ')') {
            p->error = CALC_ERR_SYNTAX;
            return 0.0;
        }
        get(p); /* Consume ')' */
        return val;
    }

    /* Number */
    if (isdigit((unsigned char)c) || c == '.') {
        char *endptr;
        const char *start = &p->src[p->pos];
        double val = strtod(start, &endptr);
        if (endptr == start) {
            p->error = CALC_ERR_SYNTAX;
            return 0.0;
        }
        p->pos += (size_t)(endptr - start);
        return val;
    }

    p->error = CALC_ERR_SYNTAX;
    return 0.0;
}

static double parse_power(Parser *p) {
    double left = parse_factor(p);
    if (p->error != CALC_OK) return 0.0;

    skip_whitespace(p);
    if (peek(p) == '^') {
        get(p);
        double right = parse_power(p); /* Right-associative */
        if (p->error != CALC_OK) return 0.0;
        if (left < 0.0 && floor(right) != right) {
            p->error = CALC_ERR_OVERFLOW;
            return 0.0;
        }
        double res = pow(left, right);
        if (isnan(res) || isinf(res)) {
            p->error = CALC_ERR_OVERFLOW;
            return 0.0;
        }
        return res;
    }

    return left;
}

static double parse_term(Parser *p) {
    double left = parse_power(p);
    if (p->error != CALC_OK) return 0.0;

    for (;;) {
        char op = peek(p);
        if (op != '*' && op != '/' && op != '%' && op != 'x' && op != 'X') {
            break;
        }
        get(p);
        double right = parse_power(p);
        if (p->error != CALC_OK) return 0.0;

        if (op == '/' || op == '%') {
            if (fabs(right) < 1e-12) {
                p->error = CALC_ERR_DIV_ZERO;
                return 0.0;
            }
            if (op == '/') left /= right;
            else left = fmod(left, right);
        } else {
            left *= right;
        }
    }

    return left;
}

static double parse_expr(Parser *p) {
    double left = parse_term(p);
    if (p->error != CALC_OK) return 0.0;

    for (;;) {
        char op = peek(p);
        if (op != '+' && op != '-') {
            break;
        }
        get(p);
        double right = parse_term(p);
        if (p->error != CALC_OK) return 0.0;

        if (op == '+') left += right;
        else left -= right;
    }

    return left;
}

CalcStatus calc_evaluate(const char *expr, double *result) {
    if (!expr || !result) return CALC_ERR_SYNTAX;

    /* Check for empty/whitespace-only input */
    const char *cur = expr;
    while (*cur != '\0' && isspace((unsigned char)*cur)) cur++;
    if (*cur == '\0') return CALC_ERR_EMPTY;

    Parser p = { .src = expr, .pos = 0, .error = CALC_OK };
    double val = parse_expr(&p);

    if (p.error != CALC_OK) return p.error;

    skip_whitespace(&p);
    if (p.src[p.pos] != '\0') {
        return CALC_ERR_SYNTAX; /* Trailing unparsed tokens */
    }

    *result = val;
    return CALC_OK;
}

void calc_add_history(Calculator *calc, const char *expr, double result) {
    if (!calc || !expr) return;

    if (calc->history_count < CALC_HISTORY_MAX) {
        snprintf(calc->history[calc->history_count].expression,
                 sizeof(calc->history[calc->history_count].expression),
                 "%s", expr);
        calc->history[calc->history_count].result = result;
        calc->history_count++;
    } else {
        /* Shift older entries */
        for (int i = 1; i < CALC_HISTORY_MAX; i++) {
            calc->history[i - 1] = calc->history[i];
        }
        snprintf(calc->history[CALC_HISTORY_MAX - 1].expression,
                 sizeof(calc->history[CALC_HISTORY_MAX - 1].expression),
                 "%s", expr);
        calc->history[CALC_HISTORY_MAX - 1].result = result;
    }
}

void calc_clear_history(Calculator *calc) {
    if (!calc) return;
    calc->history_count = 0;
}
