#include "contact.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

/* ------------------------------------------------------------------ */
/*  RFC 4180 CSV Formatting & Escaping                                */
/* ------------------------------------------------------------------ */

static void write_csv_escaped_field(FILE *fp, const char *field) {
    if (!field || field[0] == '\0') {
        return;
    }

    bool needs_quotes = false;
    for (size_t i = 0; field[i] != '\0'; i++) {
        if (field[i] == ',' || field[i] == '"' || field[i] == '\n' || field[i] == '\r') {
            needs_quotes = true;
            break;
        }
    }

    if (!needs_quotes) {
        fputs(field, fp);
        return;
    }

    fputc('"', fp);
    for (size_t i = 0; field[i] != '\0'; i++) {
        if (field[i] == '"') {
            fputc('"', fp); /* Escape quote by doubling */
        }
        fputc(field[i], fp);
    }
    fputc('"', fp);
}

bool contact_export_csv(const ContactBook *book, const char *filepath) {
    if (!book || !filepath) return false;

    FILE *fp = fopen(filepath, "w");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open '%s' for CSV export: ", filepath);
        perror("");
        return false;
    }

    /* Standard CSV header */
    fprintf(fp, "Name,Phone,Email,Address,Category\n");

    for (int i = 0; i < book->count; i++) {
        const Contact *c = &book->items[i];
        if (c->is_deleted) continue;

        write_csv_escaped_field(fp, c->name);
        fputc(',', fp);
        write_csv_escaped_field(fp, c->phone);
        fputc(',', fp);
        write_csv_escaped_field(fp, c->email);
        fputc(',', fp);
        write_csv_escaped_field(fp, c->address);
        fputc(',', fp);
        write_csv_escaped_field(fp, c->category);
        fputc('\n', fp);
    }

    fclose(fp);
    return true;
}

/* ------------------------------------------------------------------ */
/*  RFC 4180 CSV Parsing                                              */
/* ------------------------------------------------------------------ */

/* Parse a single CSV line into up to 5 fields */
static int parse_csv_line(const char *line, char fields[5][128]) {
    for (int i = 0; i < 5; i++) {
        fields[i][0] = '\0';
    }

    size_t col = 0;
    size_t out_idx = 0;
    bool in_quotes = false;

    for (size_t i = 0; line[i] != '\0' && line[i] != '\r' && line[i] != '\n'; i++) {
        char ch = line[i];

        if (in_quotes) {
            if (ch == '"') {
                if (line[i + 1] == '"') {
                    /* Escaped quote */
                    if (col < 5 && out_idx + 1 < 128) {
                        fields[col][out_idx++] = '"';
                    }
                    i++; /* Skip next quote */
                } else {
                    /* Closing quote */
                    in_quotes = false;
                }
            } else {
                if (col < 5 && out_idx + 1 < 128) {
                    fields[col][out_idx++] = ch;
                }
            }
        } else {
            if (ch == '"') {
                in_quotes = true;
            } else if (ch == ',') {
                if (col < 5) {
                    fields[col][out_idx] = '\0';
                }
                col++;
                out_idx = 0;
            } else {
                if (col < 5 && out_idx + 1 < 128) {
                    fields[col][out_idx++] = ch;
                }
            }
        }
    }

    if (col < 5) {
        fields[col][out_idx] = '\0';
        col++;
    }

    return (int)col;
}

static void trim_field(char *str) {
    if (!str) return;
    char *start = str;
    while (*start != '\0' && isspace((unsigned char)*start)) {
        start++;
    }
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
    size_t len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[--len] = '\0';
    }
}

bool contact_import_csv(ContactBook *book, const char *filepath, int *imported_count, int *skipped_count) {
    if (!book || !filepath) return false;

    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open '%s' for CSV import: ", filepath);
        perror("");
        return false;
    }

    int imported = 0;
    int skipped = 0;
    char line[1024];
    bool is_first_line = true;

    while (fgets(line, sizeof(line), fp) != NULL) {
        char fields[5][128];
        int num_cols = parse_csv_line(line, fields);
        if (num_cols < 3) {
            /* Line is blank or has fewer than required columns (name, phone, email) */
            continue;
        }

        for (int i = 0; i < 5; i++) {
            trim_field(fields[i]);
        }

        /* Check for header row */
        if (is_first_line) {
            is_first_line = false;
            if (strcasecmp(fields[0], "name") == 0 &&
                strcasecmp(fields[1], "phone") == 0 &&
                strcasecmp(fields[2], "email") == 0) {
                continue;
            }
        }

        const char *name = fields[0];
        const char *phone = fields[1];
        const char *email = fields[2];
        const char *addr = fields[3];
        const char *cat = fields[4];

        if (name[0] == '\0' || phone[0] == '\0' || email[0] == '\0') {
            skipped++;
            continue;
        }

        int new_id = contact_add(book, name, phone, email, addr, cat);
        if (new_id > 0) {
            imported++;
        } else {
            skipped++;
        }
    }

    fclose(fp);

    if (imported_count) *imported_count = imported;
    if (skipped_count)  *skipped_count = skipped;
    return true;
}
