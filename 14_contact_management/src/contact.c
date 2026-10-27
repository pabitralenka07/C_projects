#include "contact.h"
#include "safe_utils.h"
#include "file_ops.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>

#define INITIAL_CAPACITY 16

/* ------------------------------------------------------------------ */
/*  Validation Helpers                                                */
/* ------------------------------------------------------------------ */

bool contact_validate_phone(const char *phone) {
    if (!phone) return false;
    size_t len = strlen(phone);
    if (len < 7 || len >= CONTACT_PHONE_LEN) return false;

    int digit_count = 0;
    for (size_t i = 0; i < len; i++) {
        char c = phone[i];
        if (isdigit((unsigned char)c)) {
            digit_count++;
        } else if (c == '+' && i == 0) {
            /* Leading plus is allowed */
            continue;
        } else if (c == '-' || c == ' ' || c == '(' || c == ')') {
            /* Standard phone separators */
            continue;
        } else {
            /* Invalid character in phone string */
            return false;
        }
    }

    return (digit_count >= 7 && digit_count <= 15);
}

bool contact_validate_email(const char *email) {
    if (!email) return false;
    size_t len = strlen(email);
    if (len < 5 || len >= CONTACT_EMAIL_LEN) return false;

    const char *at = strchr(email, '@');
    if (!at || at == email) {
        /* Missing '@' or empty local-part */
        return false;
    }

    /* Verify there is only one '@' */
    if (strchr(at + 1, '@') != NULL) {
        return false;
    }

    const char *domain = at + 1;
    size_t domain_len = strlen(domain);
    if (domain_len < 3) return false;

    const char *dot = strchr(domain, '.');
    if (!dot || dot == domain || dot == domain + domain_len - 1) {
        /* Domain must contain at least one '.' and cannot start or end with '.' */
        return false;
    }

    /* Check for whitespace or forbidden characters */
    for (size_t i = 0; i < len; i++) {
        if (isspace((unsigned char)email[i]) || email[i] == ',' || email[i] == ';') {
            return false;
        }
    }

    return true;
}

/* Extract only digits from phone string for normalized comparison */
static void normalize_phone_digits(const char *in, char *out, size_t out_len) {
    size_t j = 0;
    for (size_t i = 0; in[i] != '\0' && j + 1 < out_len; i++) {
        if (isdigit((unsigned char)in[i])) {
            out[j++] = in[i];
        }
    }
    out[j] = '\0';
}

int contact_find_duplicate_phone(const ContactBook *book, const char *phone, int exclude_id) {
    if (!book || !phone) return 0;
    char norm1[CONTACT_PHONE_LEN];
    normalize_phone_digits(phone, norm1, sizeof(norm1));

    for (int i = 0; i < book->count; i++) {
        const Contact *c = &book->items[i];
        if (c->is_deleted || c->id == exclude_id) continue;

        char norm2[CONTACT_PHONE_LEN];
        normalize_phone_digits(c->phone, norm2, sizeof(norm2));
        if (strcmp(norm1, norm2) == 0) {
            return c->id;
        }
    }
    return 0;
}

int contact_find_duplicate_email(const ContactBook *book, const char *email, int exclude_id) {
    if (!book || !email) return 0;

    for (int i = 0; i < book->count; i++) {
        const Contact *c = &book->items[i];
        if (c->is_deleted || c->id == exclude_id) continue;

        if (strcasecmp(c->email, email) == 0) {
            return c->id;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Lifecycle & Storage                                               */
/* ------------------------------------------------------------------ */

void contact_book_init(ContactBook *book) {
    if (!book) return;
    memset(book, 0, sizeof(ContactBook));
    book->next_id = 1;

    const char *env = getenv("DATA_DIR");
    if (env && env[0] != '\0') {
        snprintf(book->data_dir, sizeof(book->data_dir), "%s", env);
    } else {
        snprintf(book->data_dir, sizeof(book->data_dir), "./data");
    }
}

void contact_book_free(ContactBook *book) {
    if (!book) return;
    free(book->items);
    memset(book, 0, sizeof(ContactBook));
}

bool contact_book_load(ContactBook *book) {
    if (!book) return false;
    if (!util_ensure_dir(book->data_dir)) {
        fprintf(stderr, "Error: Cannot create data directory '%s'.\n", book->data_dir);
        return false;
    }

    char path[512];
    snprintf(path, sizeof(path), "%s/%s", book->data_dir, CONTACT_DATA_FILE);

    void *data = NULL;
    uint32_t count = 0, next_id = 1;
    int res = file_validated_load(path, CONTACT_MAGIC, &data, sizeof(Contact), &count, &next_id);

    if (res == FILE_LOAD_CORRUPT) {
        printf("\n  WARNING: Contacts data file '%s' is corrupted!\n", path);
        if (safe_confirm("  Attempt recovery from backup?")) {
            if (safe_recover_backup(path, CONTACT_MAGIC, sizeof(Contact))) {
                res = file_validated_load(path, CONTACT_MAGIC, &data, sizeof(Contact), &count, &next_id);
                if (res == FILE_LOAD_OK) {
                    printf("  Recovery successful — loaded %u contact record(s).\n", count);
                }
            }
        }
        if (res != FILE_LOAD_OK) {
            printf("  Starting with empty contact database.\n");
            data = NULL;
            count = 0;
            next_id = 1;
        }
    } else if (res == FILE_LOAD_FRESH) {
        printf("  No contacts data file found — starting fresh.\n");
    } else if (res == FILE_LOAD_OK && count > 0) {
        printf("  Loaded %u contact record(s).\n", count);
    }

    book->items = (Contact *)data;
    book->count = (int)count;
    book->capacity = (int)count;
    book->next_id = (int)next_id;
    return true;
}

bool contact_book_save(ContactBook *book) {
    if (!book) return false;
    if (!util_ensure_dir(book->data_dir)) {
        return false;
    }

    char path[512];
    snprintf(path, sizeof(path), "%s/%s", book->data_dir, CONTACT_DATA_FILE);

    return safe_atomic_save(path, CONTACT_MAGIC, book->items, sizeof(Contact),
                            (uint32_t)book->count, (uint32_t)book->next_id);
}

/* ------------------------------------------------------------------ */
/*  CRUD Operations                                                   */
/* ------------------------------------------------------------------ */

static bool ensure_capacity(ContactBook *book) {
    if (book->count < book->capacity) return true;

    int new_cap = (book->capacity == 0) ? INITIAL_CAPACITY : book->capacity * 2;
    Contact *new_items = (Contact *)realloc(book->items, (size_t)new_cap * sizeof(Contact));
    if (!new_items) {
        fprintf(stderr, "Error: Memory allocation failed for ContactBook.\n");
        return false;
    }

    book->items = new_items;
    book->capacity = new_cap;
    return true;
}

int contact_add(ContactBook *book, const char *name, const char *phone,
                const char *email, const char *address, const char *category) {
    if (!book || !name || !phone || !email) return -1;

    if (!contact_validate_phone(phone)) {
        fprintf(stderr, "Error: Invalid phone number format.\n");
        return -1;
    }

    if (!contact_validate_email(email)) {
        fprintf(stderr, "Error: Invalid email address format.\n");
        return -1;
    }

    if (contact_find_duplicate_phone(book, phone, 0) != 0) {
        fprintf(stderr, "Error: A contact with phone number '%s' already exists.\n", phone);
        return -1;
    }

    if (contact_find_duplicate_email(book, email, 0) != 0) {
        fprintf(stderr, "Error: A contact with email address '%s' already exists.\n", email);
        return -1;
    }

    if (!ensure_capacity(book)) return -1;

    Contact *c = &book->items[book->count];
    /* Zero the struct completely before setting fields to avoid disk padding leaks */
    memset(c, 0, sizeof(Contact));

    c->id = book->next_id++;
    strncpy(c->name, name, sizeof(c->name) - 1);
    strncpy(c->phone, phone, sizeof(c->phone) - 1);
    strncpy(c->email, email, sizeof(c->email) - 1);
    if (address) {
        strncpy(c->address, address, sizeof(c->address) - 1);
    }
    if (category && category[0] != '\0') {
        strncpy(c->category, category, sizeof(c->category) - 1);
    } else {
        strncpy(c->category, "General", sizeof(c->category) - 1);
    }
    c->is_deleted = 0;

    book->count++;
    return c->id;
}

bool contact_update(ContactBook *book, int id, const char *name, const char *phone,
                    const char *email, const char *address, const char *category) {
    Contact *c = contact_find_by_id(book, id);
    if (!c) return false;

    if (phone && phone[0] != '\0') {
        if (!contact_validate_phone(phone)) {
            fprintf(stderr, "Error: Invalid phone number format.\n");
            return false;
        }
        if (contact_find_duplicate_phone(book, phone, id) != 0) {
            fprintf(stderr, "Error: Another contact with phone '%s' already exists.\n", phone);
            return false;
        }
        strncpy(c->phone, phone, sizeof(c->phone) - 1);
    }

    if (email && email[0] != '\0') {
        if (!contact_validate_email(email)) {
            fprintf(stderr, "Error: Invalid email format.\n");
            return false;
        }
        if (contact_find_duplicate_email(book, email, id) != 0) {
            fprintf(stderr, "Error: Another contact with email '%s' already exists.\n", email);
            return false;
        }
        strncpy(c->email, email, sizeof(c->email) - 1);
    }

    if (name && name[0] != '\0') {
        strncpy(c->name, name, sizeof(c->name) - 1);
    }
    if (address) {
        strncpy(c->address, address, sizeof(c->address) - 1);
    }
    if (category && category[0] != '\0') {
        strncpy(c->category, category, sizeof(c->category) - 1);
    }

    return true;
}

bool contact_delete(ContactBook *book, int id) {
    Contact *c = contact_find_by_id(book, id);
    if (!c) return false;
    c->is_deleted = 1;
    return true;
}

Contact *contact_find_by_id(ContactBook *book, int id) {
    if (!book) return NULL;
    for (int i = 0; i < book->count; i++) {
        if (!book->items[i].is_deleted && book->items[i].id == id) {
            return &book->items[i];
        }
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/*  Search and Sort                                                   */
/* ------------------------------------------------------------------ */

static bool case_insensitive_contains(const char *haystack, const char *needle) {
    if (!haystack || !needle) return false;
    if (needle[0] == '\0') return true;

    size_t nlen = strlen(needle);
    size_t hlen = strlen(haystack);
    if (nlen > hlen) return false;

    for (size_t i = 0; i <= hlen - nlen; i++) {
        if (strncasecmp(&haystack[i], needle, nlen) == 0) {
            return true;
        }
    }
    return false;
}

int contact_search(const ContactBook *book, const char *query, int *results, int max_results) {
    if (!book || !query || !results || max_results <= 0) return 0;
    int found = 0;

    for (int i = 0; i < book->count && found < max_results; i++) {
        const Contact *c = &book->items[i];
        if (c->is_deleted) continue;

        if (case_insensitive_contains(c->name, query) ||
            case_insensitive_contains(c->phone, query) ||
            case_insensitive_contains(c->email, query) ||
            case_insensitive_contains(c->category, query)) {
            results[found++] = c->id;
        }
    }
    return found;
}

static int cmp_name_asc(const void *a, const void *b) {
    const Contact *ca = (const Contact *)a;
    const Contact *cb = (const Contact *)b;
    return strcasecmp(ca->name, cb->name);
}

static int cmp_name_desc(const void *a, const void *b) {
    const Contact *ca = (const Contact *)a;
    const Contact *cb = (const Contact *)b;
    return strcasecmp(cb->name, ca->name);
}

static int cmp_id_asc(const void *a, const void *b) {
    const Contact *ca = (const Contact *)a;
    const Contact *cb = (const Contact *)b;
    return (ca->id - cb->id);
}

void contact_sort(ContactBook *book, ContactSortOrder order) {
    if (!book || book->count < 2) return;

    switch (order) {
        case SORT_NAME_ASC:
            qsort(book->items, (size_t)book->count, sizeof(Contact), cmp_name_asc);
            break;
        case SORT_NAME_DESC:
            qsort(book->items, (size_t)book->count, sizeof(Contact), cmp_name_desc);
            break;
        case SORT_ID_ASC:
        default:
            qsort(book->items, (size_t)book->count, sizeof(Contact), cmp_id_asc);
            break;
    }
}

void contact_list_all(const ContactBook *book) {
    if (!book) return;

    printf("\n  +----+----------------------+-------------------+-----------------------+------------+\n");
    printf("  | ID | Name                 | Phone             | Email                 | Category   |\n");
    printf("  +----+----------------------+-------------------+-----------------------+------------+\n");

    int active = 0;
    for (int i = 0; i < book->count; i++) {
        const Contact *c = &book->items[i];
        if (c->is_deleted) continue;
        printf("  | %-2d | %-20.20s | %-17.17s | %-21.21s | %-10.10s |\n",
               c->id, c->name, c->phone, c->email, c->category);
        active++;
    }

    printf("  +----+----------------------+-------------------+-----------------------+------------+\n");
    printf("  Total active contacts: %d\n\n", active);
}
