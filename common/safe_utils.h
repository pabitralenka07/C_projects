#ifndef COMMON_SAFE_UTILS_H
#define COMMON_SAFE_UTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "file_ops.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Terminal and display helpers:
 * Clears terminal screen using ANSI escape sequences only when stdout is a TTY.
 * Avoids polluting pipes, headless integration tests, or redirected output.
 */
void safe_clear_screen(void);

/*
 * Safe numeric input utilities:
 * Uses fgets + strtol/strtod only.
 * Rejects NaN, +Inf, -Inf using `!(val >= min && val <= max)`.
 * Re-prompts on invalid input; returns false on EOF.
 */
bool safe_read_int(const char *prompt, int *out, int min, int max);
bool safe_read_double(const char *prompt, double *out, double min, double max);

/*
 * Safe string input:
 * Reads a line using fgets into buf (up to len characters including NUL).
 * If the input exceeds the buffer capacity, excess characters are drained from
 * stdin and the function rejects the over-long input with an informative warning,
 * re-prompting the user instead of silently truncating the data.
 * Returns true on successful input, false on EOF or stream error.
 */
bool safe_read_string(const char *prompt, char *buf, size_t len, bool allow_empty);

/*
 * Confirmation prompt:
 * Displays `prompt (y/n): `.
 * Returns true for 'y'/'Y', false for 'n'/'N'.
 * On EOF or stream error, strictly returns false ("EOF must mean no").
 */
bool safe_confirm(const char *prompt);

/*
 * Menu choice reader:
 * Displays "Enter your choice: " and reads an integer in [min, max].
 * Returns the selected choice, or -1 on EOF.
 */
int safe_menu_choice(int min, int max);

/*
 * Crash-safe atomic persistence with backup preservation:
 * 1. Writes payload to <path>.tmp and flushes + fsyncs.
 * 2. If <path> already exists on disk, inspects its header (magic, version, size match).
 *    ONLY if the existing file is verified valid does it copy <path> to <path>.bak.
 *    If <path> is corrupted, it leaves <path>.bak untouched to prevent destroying a good backup.
 * 3. Atomically renames <path>.tmp to <path>.
 * Returns true on success.
 */
bool safe_atomic_save(const char *path, const char *magic,
                      const void *records, size_t record_size,
                      uint32_t count, uint32_t next_id);

/*
 * Backup recovery with validation:
 * Validates that <path>.bak is a valid uncorrupted data file before restoring it over <path>.
 * Returns true on successful restoration.
 */
bool safe_recover_backup(const char *path, const char *magic, size_t record_size);

#ifdef __cplusplus
}
#endif

#endif /* COMMON_SAFE_UTILS_H */
