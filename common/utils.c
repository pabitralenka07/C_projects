#include "utils.h"
#include "input.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(p) _mkdir(p)
#else
    #define MKDIR(p) mkdir((p), 0755)
#endif

void util_clear_screen(void) {
    /* ANSI escape sequence — works on macOS, Linux, and modern Windows terminals.
     * Avoids system("clear"/"cls") for portability and security. */
#ifdef _WIN32
    /* Windows cmd.exe may not support ANSI, but Windows Terminal does.
     * Fall back to system("cls") on older Windows. */
    if (system("cls") != 0) {
        printf("\033[2J\033[H");
    }
#else
    printf("\033[2J\033[H");
    fflush(stdout);
#endif
}

void util_press_enter(void) {
    printf("\nPress Enter to continue...");
    fflush(stdout);
    char buf[8];
    input_read_line(buf, sizeof(buf), stdin);
}

void util_print_header(const char *title) {
    int len = (int)strlen(title);
    int width = len + 6;

    printf("  ");
    for (int i = 0; i < width; i++) printf("=");
    printf("\n");

    printf("  || %s ||\n", title);

    printf("  ");
    for (int i = 0; i < width; i++) printf("=");
    printf("\n");
}

void util_print_separator(int width) {
    printf("  ");
    for (int i = 0; i < width; i++) {
        printf("-");
    }
    printf("\n");
}

bool util_ensure_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        /* Path exists — check if it's a directory */
        return S_ISDIR(st.st_mode);
    }

    /* Directory doesn't exist — create it */
    if (MKDIR(path) != 0) {
        perror("Error creating directory");
        return false;
    }
    return true;
}
