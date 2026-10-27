#ifndef COMMON_INPUT_H
#define COMMON_INPUT_H

#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>

/*
 * Safe input utilities — wraps fgets + strtol/strtod.
 * Never uses bare scanf for strings. Every function handles
 * EOF, empty input, overflow, and over-long lines.
 */

/* Read a line from stream into buf (max len including NUL).
 * Strips trailing newline. Drains excess chars if line is too long.
 * Returns true on success, false on EOF or error. */
bool input_read_line(char *buf, size_t len, FILE *stream);

/* Prompt and read an int in [min, max]. Re-prompts until valid.
 * Returns true on success, false on EOF. */
bool input_read_int(const char *prompt, int *out, int min, int max);

/* Prompt and read a double in [min, max]. Re-prompts until valid.
 * Returns true on success, false on EOF. */
bool input_read_double(const char *prompt, double *out, double min, double max);

/* Prompt and read a non-empty trimmed string. Re-prompts if empty.
 * Returns true on success, false on EOF. */
bool input_read_string(const char *prompt, char *buf, size_t len);

/* Read a menu choice in [min, max]. Returns the choice, or -1 on EOF. */
int input_menu_choice(int min, int max);

/* Ask y/n confirmation. Returns true for yes, false for no.
 * Re-prompts on invalid input. Returns default_val on EOF. */
bool input_confirm(const char *prompt, bool default_val);

/* Trim leading and trailing whitespace in-place. */
void input_trim(char *str);

#endif /* COMMON_INPUT_H */
