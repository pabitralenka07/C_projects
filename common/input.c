#include "input.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>

void input_trim(char *str) {
    if (!str) return;

    /* Trim leading whitespace */
    char *start = str;
    while (isspace((unsigned char)*start)) {
        start++;
    }
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }

    /* Trim trailing whitespace */
    size_t len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[--len] = '\0';
    }
}

bool input_read_line(char *buf, size_t len, FILE *stream) {
    if (len == 0 || !buf || !stream) {
        return false;
    }

    if (fgets(buf, (int)len, stream) == NULL) {
        buf[0] = '\0';
        return false;
    }

    size_t slen = strlen(buf);
    if (slen > 0 && buf[slen - 1] == '\n') {
        buf[slen - 1] = '\0';
    } else {
        /* Line was longer than buffer — drain remaining chars
         * to avoid leaving stale input for the next read. */
        int ch;
        while ((ch = fgetc(stream)) != '\n' && ch != EOF) {
            /* discard */
        }
    }

    return true;
}

bool input_read_int(const char *prompt, int *out, int min, int max) {
    char buf[64];

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);

        if (!input_read_line(buf, sizeof(buf), stdin)) {
            return false;
        }

        input_trim(buf);
        if (buf[0] == '\0') {
            printf("  Input cannot be empty. Please try again.\n");
            continue;
        }

        char *endptr;
        errno = 0;
        long val = strtol(buf, &endptr, 10);

        if (endptr == buf || *endptr != '\0') {
            printf("  Invalid number. Please enter a valid integer.\n");
            continue;
        }

        if (errno == ERANGE || val < INT_MIN || val > INT_MAX) {
            printf("  Number out of range.\n");
            continue;
        }

        if ((int)val < min || (int)val > max) {
            printf("  Please enter a number between %d and %d.\n", min, max);
            continue;
        }

        *out = (int)val;
        return true;
    }
}

bool input_read_double(const char *prompt, double *out, double min, double max) {
    char buf[64];

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);

        if (!input_read_line(buf, sizeof(buf), stdin)) {
            return false;
        }

        input_trim(buf);
        if (buf[0] == '\0') {
            printf("  Input cannot be empty. Please try again.\n");
            continue;
        }

        char *endptr;
        errno = 0;
        double val = strtod(buf, &endptr);

        if (endptr == buf || *endptr != '\0') {
            printf("  Invalid number. Please enter a valid number.\n");
            continue;
        }

        if (errno == ERANGE) {
            printf("  Number out of range.\n");
            continue;
        }

        if (val < min || val > max) {
            printf("  Please enter a number between %.2f and %.2f.\n", min, max);
            continue;
        }

        *out = val;
        return true;
    }
}

bool input_read_string(const char *prompt, char *buf, size_t len) {
    for (;;) {
        printf("%s", prompt);
        fflush(stdout);

        if (!input_read_line(buf, len, stdin)) {
            return false;
        }

        input_trim(buf);
        if (buf[0] == '\0') {
            printf("  Input cannot be empty. Please try again.\n");
            continue;
        }

        return true;
    }
}

int input_menu_choice(int min, int max) {
    int choice;
    if (!input_read_int("Enter your choice: ", &choice, min, max)) {
        return -1;
    }
    return choice;
}

bool input_confirm(const char *prompt, bool default_val) {
    char buf[16];

    for (;;) {
        printf("%s (y/n): ", prompt);
        fflush(stdout);

        if (!input_read_line(buf, sizeof(buf), stdin)) {
            return default_val;
        }

        input_trim(buf);

        if (buf[0] == 'y' || buf[0] == 'Y') {
            return true;
        }
        if (buf[0] == 'n' || buf[0] == 'N') {
            return false;
        }

        printf("  Please enter 'y' or 'n'.\n");
    }
}
