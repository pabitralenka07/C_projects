#include "dict.h"
#include "safe_utils.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void menu_exact_lookup(const Dictionary *dict) {
    printf("\n--- Exact Word Lookup (Binary Search O(log N)) ---\n");
    char word[DICT_WORD_LEN];
    if (!safe_read_string("Enter word to look up: ", word, sizeof(word), false)) return;

    DictEntry *entry = dict_binary_search(dict, word);
    if (entry) {
        dict_print_entry(entry);
    } else {
        printf("\n  Word '%s' was not found in dictionary.\n", word);
    }
}

static void menu_prefix_search(const Dictionary *dict) {
    printf("\n--- Prefix Search (Autocomplete) ---\n");
    char prefix[DICT_WORD_LEN];
    if (!safe_read_string("Enter prefix to match (e.g. 'com', 'con', 'de'): ", prefix, sizeof(prefix), false)) return;

    int results[64];
    int count = dict_prefix_search(dict, prefix, results, 64);

    if (count == 0) {
        printf("\n  No words found starting with '%s'.\n", prefix);
        return;
    }

    printf("\nFound %d words with prefix '%s':\n", count, prefix);
    for (int i = 0; i < count; i++) {
        const DictEntry *e = &dict->entries[results[i]];
        printf("  • %-16s (%s): %s\n", e->word, e->part_of_speech, e->meaning);
    }
}

static void menu_substring_search(const Dictionary *dict) {
    printf("\n--- Substring & Keyword Search ---\n");
    char sub[64];
    if (!safe_read_string("Enter search keyword: ", sub, sizeof(sub), false)) return;

    int results[64];
    int count = dict_substring_search(dict, sub, results, 64);

    if (count == 0) {
        printf("\n  No dictionary entries matched '%s'.\n", sub);
        return;
    }

    printf("\nFound %d matching entries for '%s':\n", count, sub);
    for (int i = 0; i < count; i++) {
        const DictEntry *e = &dict->entries[results[i]];
        printf("  • %-16s (%s): %s\n", e->word, e->part_of_speech, e->meaning);
    }
}

static void menu_word_of_the_day(const Dictionary *dict) {
    printf("\n--- Word of the Day ---\n");
    const DictEntry *entry = dict_get_word_of_the_day(dict, 0);
    if (entry) {
        printf("  🌟 Featured Vocabulary for Today:\n");
        dict_print_entry(entry);
    } else {
        printf("  Dictionary is currently empty.\n");
    }
}

static void menu_add_word(Dictionary *dict) {
    printf("\n--- Add New Vocabulary Word ---\n");

    char word[DICT_WORD_LEN];
    if (!safe_read_string("Enter word: ", word, sizeof(word), false)) return;

    char pos[DICT_POS_LEN];
    if (!safe_read_string("Enter part of speech (noun, verb, adjective, etc.): ", pos, sizeof(pos), false)) return;

    char meaning[DICT_MEAN_LEN];
    if (!safe_read_string("Enter meaning/definition: ", meaning, sizeof(meaning), false)) return;

    char example[DICT_EX_LEN];
    if (!safe_read_string("Enter example sentence (or press Enter to skip): ", example, sizeof(example), true)) return;

    int id = dict_add(dict, word, pos, meaning, example);
    if (id > 0) {
        printf("\n  [SUCCESS] Word '%s' added with ID #%d!\n", word, id);
        dict_save(dict);
    } else {
        printf("\n  [FAILED] Word '%s' already exists in dictionary.\n", word);
    }
}

static void menu_edit_word(Dictionary *dict) {
    printf("\n--- Edit Word Definition ---\n");
    char word[DICT_WORD_LEN];
    if (!safe_read_string("Enter word to edit: ", word, sizeof(word), false)) return;

    DictEntry *entry = dict_binary_search(dict, word);
    if (!entry) {
        printf("  Word '%s' not found.\n", word);
        return;
    }

    printf("Editing '%s'. Leave blank to keep current value.\n", entry->word);

    char pos[DICT_POS_LEN];
    printf("Current Part of Speech [%s]: ", entry->part_of_speech);
    if (!safe_read_string("", pos, sizeof(pos), true)) return;

    char meaning[DICT_MEAN_LEN];
    printf("Current Meaning [%s]: ", entry->meaning);
    if (!safe_read_string("", meaning, sizeof(meaning), true)) return;

    char example[DICT_EX_LEN];
    printf("Current Example [%s]: ", entry->example);
    if (!safe_read_string("", example, sizeof(example), true)) return;

    if (dict_update(dict, word,
                    pos[0] ? pos : NULL,
                    meaning[0] ? meaning : NULL,
                    example[0] ? example : NULL)) {
        printf("\n  [SUCCESS] Word '%s' definition updated successfully!\n", word);
    } else {
        printf("\n  [FAILED] Could not update word.\n");
    }
}

static void menu_delete_word(Dictionary *dict) {
    printf("\n--- Delete Word ---\n");
    char word[DICT_WORD_LEN];
    if (!safe_read_string("Enter word to delete: ", word, sizeof(word), false)) return;

    DictEntry *entry = dict_binary_search(dict, word);
    if (!entry) {
        printf("  Word '%s' not found.\n", word);
        return;
    }

    if (safe_confirm("Are you sure you want to delete this word from dictionary?")) {
        dict_delete(dict, word);
        printf("  [SUCCESS] Word '%s' has been removed.\n", word);
    } else {
        printf("  Deletion canceled.\n");
    }
}

int main(void) {
    Dictionary dict;
    dict_init(&dict);
    dict_load(&dict);

    bool running = true;
    while (running) {
        safe_clear_screen();
        printf("===============================================\n");
        printf("           CS & English Dictionary             \n");
        printf("===============================================\n");
        printf("  1. Exact Word Lookup (Binary Search)\n");
        printf("  2. Prefix Search (Autocomplete)\n");
        printf("  3. Substring & Keyword Search\n");
        printf("  4. Word of the Day\n");
        printf("  5. Add New Word\n");
        printf("  6. Edit Word Definition\n");
        printf("  7. Delete Word\n");
        printf("  8. List All Vocabulary\n");
        printf("  9. Save & Exit\n");
        printf("===============================================\n");

        int choice = safe_menu_choice(1, 9);
        if (choice == -1 || choice == 9) {
            break;
        }

        switch (choice) {
            case 1: menu_exact_lookup(&dict); break;
            case 2: menu_prefix_search(&dict); break;
            case 3: menu_substring_search(&dict); break;
            case 4: menu_word_of_the_day(&dict); break;
            case 5: menu_add_word(&dict); break;
            case 6: menu_edit_word(&dict); break;
            case 7: menu_delete_word(&dict); break;
            case 8: dict_list_all(&dict); break;
            default: break;
        }

        util_press_enter();
    }

    dict_save(&dict);
    dict_free(&dict);
    printf("\nThank you for using Dictionary App. Goodbye!\n");
    return 0;
}
