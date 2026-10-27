#include "dict.h"
#include "safe_utils.h"
#include "file_ops.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <time.h>

#define INITIAL_DICT_CAPACITY 32

/* ------------------------------------------------------------------ */
/*  Internal Helpers                                                  */
/* ------------------------------------------------------------------ */

static void to_lower_str(const char *in, char *out, size_t out_len) {
    size_t i = 0;
    while (in[i] != '\0' && i + 1 < out_len) {
        out[i] = (char)tolower((unsigned char)in[i]);
        i++;
    }
    out[i] = '\0';
}

static int cmp_entries(const void *a, const void *b) {
    const DictEntry *ea = (const DictEntry *)a;
    const DictEntry *eb = (const DictEntry *)b;
    return strcmp(ea->word, eb->word);
}

static void sort_entries(Dictionary *dict) {
    if (dict->count > 1) {
        qsort(dict->entries, (size_t)dict->count, sizeof(DictEntry), cmp_entries);
    }
}

/* ------------------------------------------------------------------ */
/*  Lifecycle & Persistence                                           */
/* ------------------------------------------------------------------ */

void dict_init(Dictionary *dict) {
    if (!dict) return;
    memset(dict, 0, sizeof(Dictionary));
    dict->next_id = 1;

    const char *env = getenv("DATA_DIR");
    if (env && env[0] != '\0') {
        snprintf(dict->data_dir, sizeof(dict->data_dir), "%s", env);
    } else {
        snprintf(dict->data_dir, sizeof(dict->data_dir), "./data");
    }
}

void dict_free(Dictionary *dict) {
    if (!dict) return;
    free(dict->entries);
    memset(dict, 0, sizeof(Dictionary));
}

int dict_load_seed_file(Dictionary *dict, const char *filepath) {
    if (!dict || !filepath) return 0;

    FILE *fp = fopen(filepath, "r");
    if (!fp) return 0;

    char line[1024];
    int loaded = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        char word[DICT_WORD_LEN] = {0};
        char pos[DICT_POS_LEN] = {0};
        char mean[DICT_MEAN_LEN] = {0};
        char ex[DICT_EX_LEN] = {0};

        /* Format: word|pos|meaning|example */
        char *p1 = strchr(line, '|');
        if (!p1) continue;
        *p1 = '\0';
        strncpy(word, line, sizeof(word) - 1);

        char *p2 = strchr(p1 + 1, '|');
        if (!p2) continue;
        *p2 = '\0';
        strncpy(pos, p1 + 1, sizeof(pos) - 1);

        char *p3 = strchr(p2 + 1, '|');
        if (!p3) continue;
        *p3 = '\0';
        strncpy(mean, p2 + 1, sizeof(mean) - 1);
        strncpy(ex, p3 + 1, sizeof(ex) - 1);

        if (word[0] != '\0' && mean[0] != '\0') {
            if (dict_add(dict, word, pos, mean, ex) > 0) {
                loaded++;
            }
        }
    }

    fclose(fp);
    return loaded;
}

bool dict_load(Dictionary *dict) {
    if (!dict) return false;
    if (!util_ensure_dir(dict->data_dir)) return false;

    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dict->data_dir, DICT_DATA_FILE);

    void *data = NULL;
    uint32_t count = 0, next_id = 1;
    int res = file_validated_load(path, DICT_MAGIC, &data, sizeof(DictEntry), &count, &next_id);

    if (res == FILE_LOAD_CORRUPT) {
        printf("\n  WARNING: Dictionary data file is corrupted!\n");
        if (safe_confirm("  Attempt recovery from backup?")) {
            if (safe_recover_backup(path, DICT_MAGIC, sizeof(DictEntry))) {
                res = file_validated_load(path, DICT_MAGIC, &data, sizeof(DictEntry), &count, &next_id);
                if (res == FILE_LOAD_OK) {
                    printf("  Recovery successful — loaded %u vocabulary entries.\n", count);
                }
            }
        }
        if (res != FILE_LOAD_OK) {
            printf("  Starting with empty dictionary.\n");
            data = NULL;
            count = 0;
            next_id = 1;
        }
    } else if (res == FILE_LOAD_FRESH) {
        /* Check if seed file exists in data_dir or parent data directory */
        char seed_path[512];
        snprintf(seed_path, sizeof(seed_path), "%s/%s", dict->data_dir, DICT_SEED_FILE);
        FILE *seed_fp = fopen(seed_path, "r");
        if (!seed_fp) {
            /* Try relative data/seed_words.txt */
            snprintf(seed_path, sizeof(seed_path), "data/%s", DICT_SEED_FILE);
            seed_fp = fopen(seed_path, "r");
        }

        if (seed_fp) {
            fclose(seed_fp);
            int seeded = dict_load_seed_file(dict, seed_path);
            if (seeded > 0) {
                printf("  Initialized dictionary with %d curated vocabulary words from seed file.\n", seeded);
                dict_save(dict);
                return true;
            }
        }
        printf("  No dictionary data file found — starting fresh.\n");
    }

    if (res == FILE_LOAD_OK && data != NULL) {
        dict->entries = (DictEntry *)data;
        dict->count = (int)count;
        dict->capacity = (int)count;
        dict->next_id = (int)next_id;
        sort_entries(dict);
    }

    return true;
}

bool dict_save(Dictionary *dict) {
    if (!dict) return false;
    if (!util_ensure_dir(dict->data_dir)) return false;

    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dict->data_dir, DICT_DATA_FILE);

    sort_entries(dict);
    return safe_atomic_save(path, DICT_MAGIC, dict->entries, sizeof(DictEntry),
                            (uint32_t)dict->count, (uint32_t)dict->next_id);
}

/* ------------------------------------------------------------------ */
/*  CRUD Operations (Sorted Order Maintained)                         */
/* ------------------------------------------------------------------ */

static bool ensure_capacity(Dictionary *dict) {
    if (dict->count < dict->capacity) return true;

    int new_cap = (dict->capacity == 0) ? INITIAL_DICT_CAPACITY : dict->capacity * 2;
    DictEntry *new_arr = (DictEntry *)realloc(dict->entries, (size_t)new_cap * sizeof(DictEntry));
    if (!new_arr) return false;

    dict->entries = new_arr;
    dict->capacity = new_cap;
    return true;
}

int dict_add(Dictionary *dict, const char *word, const char *pos,
             const char *meaning, const char *example) {
    if (!dict || !word || !meaning || word[0] == '\0') return -1;

    char lword[DICT_WORD_LEN];
    to_lower_str(word, lword, sizeof(lword));

    /* Check if word already exists */
    DictEntry *existing = dict_binary_search(dict, lword);
    if (existing) {
        if (existing->is_deleted) {
            /* Reactivate soft-deleted entry */
            existing->is_deleted = 0;
            strncpy(existing->part_of_speech, pos ? pos : "noun", sizeof(existing->part_of_speech) - 1);
            strncpy(existing->meaning, meaning, sizeof(existing->meaning) - 1);
            if (example) strncpy(existing->example, example, sizeof(existing->example) - 1);
            dict_save(dict);
            return existing->id;
        }
        return -1; /* Duplicate active word */
    }

    if (!ensure_capacity(dict)) return -1;

    DictEntry *entry = &dict->entries[dict->count];
    memset(entry, 0, sizeof(DictEntry));

    entry->id = dict->next_id++;
    strncpy(entry->word, lword, sizeof(entry->word) - 1);
    strncpy(entry->part_of_speech, pos && pos[0] ? pos : "noun", sizeof(entry->part_of_speech) - 1);
    strncpy(entry->meaning, meaning, sizeof(entry->meaning) - 1);
    if (example) {
        strncpy(entry->example, example, sizeof(entry->example) - 1);
    }
    entry->is_deleted = 0;

    dict->count++;
    sort_entries(dict);
    return entry->id;
}

bool dict_update(Dictionary *dict, const char *word, const char *pos,
                 const char *meaning, const char *example) {
    DictEntry *entry = dict_binary_search(dict, word);
    if (!entry) return false;

    if (pos && pos[0] != '\0') {
        strncpy(entry->part_of_speech, pos, sizeof(entry->part_of_speech) - 1);
    }
    if (meaning && meaning[0] != '\0') {
        strncpy(entry->meaning, meaning, sizeof(entry->meaning) - 1);
    }
    if (example && example[0] != '\0') {
        strncpy(entry->example, example, sizeof(entry->example) - 1);
    }

    dict_save(dict);
    return true;
}

bool dict_delete(Dictionary *dict, const char *word) {
    DictEntry *entry = dict_binary_search(dict, word);
    if (!entry) return false;

    entry->is_deleted = 1;
    dict_save(dict);
    return true;
}

/* ------------------------------------------------------------------ */
/*  Search Algorithms                                                 */
/* ------------------------------------------------------------------ */

DictEntry *dict_binary_search(const Dictionary *dict, const char *word) {
    if (!dict || !word || dict->count == 0) return NULL;

    char lword[DICT_WORD_LEN];
    to_lower_str(word, lword, sizeof(lword));

    int low = 0;
    int high = dict->count - 1;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        int cmp = strcmp(dict->entries[mid].word, lword);

        if (cmp == 0) {
            if (!dict->entries[mid].is_deleted) {
                return &dict->entries[mid];
            }
            return NULL; /* Found but soft-deleted */
        } else if (cmp < 0) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    return NULL;
}

int dict_prefix_search(const Dictionary *dict, const char *prefix, int *out_indices, int max_results) {
    if (!dict || !prefix || !out_indices || max_results <= 0 || dict->count == 0) return 0;

    char lprefix[DICT_WORD_LEN];
    to_lower_str(prefix, lprefix, sizeof(lprefix));
    size_t plen = strlen(lprefix);

    /* Binary search to find lower bound (first word >= lprefix) */
    int low = 0;
    int high = dict->count - 1;
    int start_idx = dict->count;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (strcmp(dict->entries[mid].word, lprefix) >= 0) {
            start_idx = mid;
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }

    int found = 0;
    for (int i = start_idx; i < dict->count && found < max_results; i++) {
        if (strncmp(dict->entries[i].word, lprefix, plen) != 0) {
            break; /* Exceeded prefix range in sorted array */
        }
        if (!dict->entries[i].is_deleted) {
            out_indices[found++] = i;
        }
    }

    return found;
}

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

int dict_substring_search(const Dictionary *dict, const char *substring, int *out_indices, int max_results) {
    if (!dict || !substring || !out_indices || max_results <= 0) return 0;

    int found = 0;

    for (int i = 0; i < dict->count && found < max_results; i++) {
        const DictEntry *e = &dict->entries[i];
        if (e->is_deleted) continue;

        if (case_insensitive_contains(e->word, substring) ||
            case_insensitive_contains(e->meaning, substring) ||
            case_insensitive_contains(e->example, substring)) {
            out_indices[found++] = i;
        }
    }

    return found;
}

/* ------------------------------------------------------------------ */
/*  Word of the Day & Display                                         */
/* ------------------------------------------------------------------ */

const DictEntry *dict_get_word_of_the_day(const Dictionary *dict, int seed_day_offset) {
    if (!dict || dict->count == 0) return NULL;

    /* Collect active entry indices */
    int active_indices[1024];
    int active_count = 0;
    for (int i = 0; i < dict->count && active_count < 1024; i++) {
        if (!dict->entries[i].is_deleted) {
            active_indices[active_count++] = i;
        }
    }
    if (active_count == 0) return NULL;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    int day_key = (t->tm_year * 1000) + t->tm_yday + seed_day_offset;
    if (day_key < 0) day_key = -day_key;

    int chosen = active_indices[day_key % active_count];
    return &dict->entries[chosen];
}

void dict_print_entry(const DictEntry *entry) {
    if (!entry || entry->is_deleted) return;

    printf("\n===============================================\n");
    printf("  Word:       %s\n", entry->word);
    printf("  Part of Speech: %s\n", entry->part_of_speech);
    printf("  Meaning:    %s\n", entry->meaning);
    if (entry->example[0] != '\0') {
        printf("  Example:    \"%s\"\n", entry->example);
    }
    printf("===============================================\n");
}

void dict_list_all(const Dictionary *dict) {
    if (!dict) return;

    printf("\n  +----+----------------------+-------------+------------------------------------------------+\n");
    printf("  | ID | Word                 | Type        | Meaning (excerpt)                              |\n");
    printf("  +----+----------------------+-------------+------------------------------------------------+\n");

    int active = 0;
    for (int i = 0; i < dict->count; i++) {
        const DictEntry *e = &dict->entries[i];
        if (e->is_deleted) continue;
        printf("  | %-2d | %-20.20s | %-11.11s | %-46.46s |\n",
               e->id, e->word, e->part_of_speech, e->meaning);
        active++;
    }

    printf("  +----+----------------------+-------------+------------------------------------------------+\n");
    printf("  Total vocabulary words: %d\n\n", active);
}
