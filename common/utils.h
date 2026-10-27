#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <stdbool.h>
#include <stddef.h>

/* Clear the terminal screen using platform-appropriate method. */
void util_clear_screen(void);

/* Print "Press Enter to continue..." and wait. */
void util_press_enter(void);

/* Print a decorated header line. */
void util_print_header(const char *title);

/* Print a horizontal separator of given width (dash characters). */
void util_print_separator(int width);

/* Ensure a directory exists, creating it if needed.
 * Returns true on success. */
bool util_ensure_dir(const char *path);

#endif /* COMMON_UTILS_H */
