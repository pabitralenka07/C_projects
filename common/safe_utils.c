#include "safe_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <sys/stat.h>

#ifdef _WIN32
    #include <io.h>
    #define isatty_fd(fd) _isatty(fd)
    #define fsync_fd(fd)  _commit(fd)
#else
    #include <unistd.h>
    #define isatty_fd(fd) isatty(fd)
    #define fsync_fd(fd)  fsync(fd)
#endif

/* ------------------------------------------------------------------ */
/*  Internal string helpers                                           */
/* ------------------------------------------------------------------ */

static void trim_whitespace(char *str) {
    if (!str) return;

    /* Trim leading whitespace */
    char *start = str;
    while (*start != '\0' && isspace((unsigned char)*start)) {
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

static char *make_suffixed_path(const char *path, const char *suffix) {
    size_t plen = strlen(path);
    size_t slen = strlen(suffix);
    char *result = (char *)malloc(plen + slen + 1);
    if (!result) return NULL;
    memcpy(result, path, plen);
    memcpy(result + plen, suffix, slen + 1);
    return result;
}

static bool copy_file_exact(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb");
    if (!in) return false;

    FILE *out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        return false;
    }

    char buf[4096];
    size_t n;
    bool ok = true;

    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) {
            ok = false;
            break;
        }
    }

    if (ferror(in)) ok = false;

    fclose(in);
    fclose(out);
    return ok;
}

/* ------------------------------------------------------------------ */
/*  Terminal display                                                  */
/* ------------------------------------------------------------------ */

void safe_clear_screen(void) {
    /* Clear using ANSI escape sequences only when stdout is attached to a terminal */
    int fd = fileno(stdout);
    if (fd >= 0 && isatty_fd(fd)) {
        printf("\033[2J\033[H");
        fflush(stdout);
    }
}

/* ------------------------------------------------------------------ */
/*  Input utilities                                                   */
/* ------------------------------------------------------------------ */

bool safe_read_string(const char *prompt, char *buf, size_t len, bool allow_empty) {
    if (!buf || len < 2) return false;

    for (;;) {
        if (prompt && prompt[0] != '\0') {
            printf("%s", prompt);
            fflush(stdout);
        }

        if (fgets(buf, (int)len, stdin) == NULL) {
            buf[0] = '\0';
            return false; /* EOF reached */
        }

        char *nl = strchr(buf, '\n');
        if (nl == NULL) {
            /* Input exceeded buffer size — drain excess characters up to newline */
            int ch;
            while ((ch = fgetc(stdin)) != '\n' && ch != EOF) {
                /* Discard */
            }
            printf("  Error: Input exceeds maximum allowed length (%zu characters). Please try again.\n",
                   len - 1);
            continue;
        }

        *nl = '\0';
        trim_whitespace(buf);

        if (!allow_empty && buf[0] == '\0') {
            printf("  Input cannot be empty. Please try again.\n");
            continue;
        }

        return true;
    }
}

bool safe_read_int(const char *prompt, int *out, int min, int max) {
    if (!out) return false;
    char buf[64];

    for (;;) {
        if (prompt && prompt[0] != '\0') {
            printf("%s", prompt);
            fflush(stdout);
        }

        if (fgets(buf, (int)sizeof(buf), stdin) == NULL) {
            return false; /* EOF */
        }

        char *nl = strchr(buf, '\n');
        if (nl == NULL) {
            int ch;
            while ((ch = fgetc(stdin)) != '\n' && ch != EOF);
            printf("  Input too long. Please enter a valid number.\n");
            continue;
        }
        *nl = '\0';
        trim_whitespace(buf);

        if (buf[0] == '\0') {
            printf("  Input cannot be empty. Please try again.\n");
            continue;
        }

        char *endptr = NULL;
        errno = 0;
        long val = strtol(buf, &endptr, 10);

        if (endptr == buf || *endptr != '\0') {
            printf("  Invalid input. Please enter an integer.\n");
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

bool safe_read_double(const char *prompt, double *out, double min, double max) {
    if (!out) return false;
    char buf[64];

    for (;;) {
        if (prompt && prompt[0] != '\0') {
            printf("%s", prompt);
            fflush(stdout);
        }

        if (fgets(buf, (int)sizeof(buf), stdin) == NULL) {
            return false; /* EOF */
        }

        char *nl = strchr(buf, '\n');
        if (nl == NULL) {
            int ch;
            while ((ch = fgetc(stdin)) != '\n' && ch != EOF);
            printf("  Input too long. Please enter a valid number.\n");
            continue;
        }
        *nl = '\0';
        trim_whitespace(buf);

        if (buf[0] == '\0') {
            printf("  Input cannot be empty. Please try again.\n");
            continue;
        }

        char *endptr = NULL;
        errno = 0;
        double val = strtod(buf, &endptr);

        if (endptr == buf || *endptr != '\0') {
            printf("  Invalid number. Please enter a numeric value.\n");
            continue;
        }

        if (errno == ERANGE) {
            printf("  Number out of floating point range.\n");
            continue;
        }

        /*
         * Critical requirement:
         * Reject NaN, +Inf, -Inf. For NaN, all comparisons return false,
         * so !(val >= min && val <= max) evaluates to true and cleanly rejects NaN.
         */
        if (!(val >= min && val <= max)) {
            printf("  Please enter a value between %.2f and %.2f (no NaN or Inf).\n", min, max);
            continue;
        }

        *out = val;
        return true;
    }
}

bool safe_confirm(const char *prompt) {
    char buf[16];

    for (;;) {
        printf("%s (y/n): ", prompt ? prompt : "Confirm?");
        fflush(stdout);

        if (fgets(buf, (int)sizeof(buf), stdin) == NULL) {
            /* Strict requirement: EOF must mean "no" */
            printf("\n");
            return false;
        }

        char *nl = strchr(buf, '\n');
        if (nl == NULL) {
            int ch;
            while ((ch = fgetc(stdin)) != '\n' && ch != EOF);
        } else {
            *nl = '\0';
        }
        trim_whitespace(buf);

        if (buf[0] == 'y' || buf[0] == 'Y') {
            return true;
        }
        if (buf[0] == 'n' || buf[0] == 'N') {
            return false;
        }

        printf("  Please enter 'y' for yes or 'n' for no.\n");
    }
}

int safe_menu_choice(int min, int max) {
    int choice = 0;
    if (!safe_read_int("Enter your choice: ", &choice, min, max)) {
        return -1;
    }
    return choice;
}

/* ------------------------------------------------------------------ */
/*  Storage integrity                                                 */
/* ------------------------------------------------------------------ */

/* Validates header and size of an existing file */
static bool is_file_healthy(const char *path, const char *magic, size_t record_size) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return false;

    FileHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fp) != 1) {
        fclose(fp);
        return false;
    }

    if (memcmp(hdr.magic, magic, FILE_MAGIC_LEN) != 0) {
        fclose(fp);
        return false;
    }

    if (hdr.version != FILE_VERSION || hdr.record_count > FILE_MAX_RECORDS) {
        fclose(fp);
        return false;
    }

    long start = ftell(fp);
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return false;
    }
    long total_size = ftell(fp);
    long expected_size = start + (long)((size_t)hdr.record_count * record_size);

    fclose(fp);
    return (total_size == expected_size);
}

bool safe_atomic_save(const char *path, const char *magic,
                      const void *records, size_t record_size,
                      uint32_t count, uint32_t next_id) {
    char *tmp_path = make_suffixed_path(path, ".tmp");
    char *bak_path = make_suffixed_path(path, ".bak");
    if (!tmp_path || !bak_path) {
        free(tmp_path);
        free(bak_path);
        return false;
    }

    /* 1. Write payload to tmp file */
    FILE *fp = fopen(tmp_path, "wb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open '%s' for writing: ", tmp_path);
        perror("");
        free(tmp_path);
        free(bak_path);
        return false;
    }

    FileHeader hdr;
    memset(&hdr, 0, sizeof(hdr));
    memcpy(hdr.magic, magic, FILE_MAGIC_LEN);
    hdr.version = FILE_VERSION;
    hdr.record_count = count;
    hdr.next_id = next_id;

    if (fwrite(&hdr, sizeof(hdr), 1, fp) != 1) {
        fprintf(stderr, "Error: Failed to write header to '%s'\n", tmp_path);
        fclose(fp);
        free(tmp_path);
        free(bak_path);
        return false;
    }

    if (count > 0 && records != NULL) {
        if (fwrite(records, record_size, count, fp) != count) {
            fprintf(stderr, "Error: Failed to write %u records to '%s'\n", count, tmp_path);
            fclose(fp);
            free(tmp_path);
            free(bak_path);
            return false;
        }
    }

    if (fflush(fp) != 0) {
        fprintf(stderr, "Error: fflush failed for '%s'\n", tmp_path);
        fclose(fp);
        free(tmp_path);
        free(bak_path);
        return false;
    }

    int fd = fileno(fp);
    if (fd >= 0) {
        (void)fsync_fd(fd);
    }
    fclose(fp);

    /*
     * 2. Validate existing file before copying to .bak:
     * If existing file is healthy, back it up.
     * If existing file is missing or corrupted, do NOT overwrite .bak!
     */
    FILE *existing = fopen(path, "rb");
    if (existing) {
        fclose(existing);
        if (is_file_healthy(path, magic, record_size)) {
            (void)copy_file_exact(path, bak_path);
        } else {
            fprintf(stderr, "  Notice: Existing file '%s' was corrupted or invalid; "
                            "preserving existing backup without overwriting.\n", path);
        }
    }

    /* 3. Atomic rename tmp -> destination */
    if (rename(tmp_path, path) != 0) {
        fprintf(stderr, "Error: Failed to atomically rename '%s' -> '%s': ", tmp_path, path);
        perror("");
        free(tmp_path);
        free(bak_path);
        return false;
    }

    free(tmp_path);
    free(bak_path);
    return true;
}

bool safe_recover_backup(const char *path, const char *magic, size_t record_size) {
    char *bak_path = make_suffixed_path(path, ".bak");
    if (!bak_path) return false;

    if (!is_file_healthy(bak_path, magic, record_size)) {
        fprintf(stderr, "Error: Backup file '%s' is missing or corrupted. Cannot recover.\n", bak_path);
        free(bak_path);
        return false;
    }

    if (!copy_file_exact(bak_path, path)) {
        fprintf(stderr, "Error: Failed to restore backup from '%s'.\n", bak_path);
        free(bak_path);
        return false;
    }

    printf("Successfully restored data from backup '%s'.\n", bak_path);
    free(bak_path);
    return true;
}
