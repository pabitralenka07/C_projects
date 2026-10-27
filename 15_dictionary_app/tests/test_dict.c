#define _DARWIN_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "dict.h"
#include "safe_utils.h"
#include "file_ops.h"

static void test_binary_search_and_crud(void) {
    Dictionary dict;
    dict_init(&dict);

    /* Insert in arbitrary order */
    assert(dict_add(&dict, "zebra", "noun", "A striped African mammal", "A zebra grazed") > 0);
    assert(dict_add(&dict, "apple", "noun", "A round edible fruit", "An apple a day") > 0);
    assert(dict_add(&dict, "mango", "noun", "A tropical stone fruit", "Sweet ripe mango") > 0);

    /* Verify array is automatically sorted lexicographically */
    assert(strcmp(dict.entries[0].word, "apple") == 0);
    assert(strcmp(dict.entries[1].word, "mango") == 0);
    assert(strcmp(dict.entries[2].word, "zebra") == 0);

    /* Binary search matches */
    DictEntry *e1 = dict_binary_search(&dict, "apple");
    assert(e1 != NULL);
    assert(strcmp(e1->meaning, "A round edible fruit") == 0);

    DictEntry *e2 = dict_binary_search(&dict, "MANGO"); /* Case-insensitive */
    assert(e2 != NULL);
    assert(strcmp(e2->word, "mango") == 0);

    assert(dict_binary_search(&dict, "banana") == NULL);

    /* Update */
    assert(dict_update(&dict, "apple", "noun", "Crisp red fruit", "Eating an apple"));
    assert(strcmp(e1->meaning, "Crisp red fruit") == 0);

    /* Delete */
    assert(dict_delete(&dict, "mango"));
    assert(dict_binary_search(&dict, "mango") == NULL);

    /* Duplicate rejection of active word */
    assert(dict_add(&dict, "apple", "noun", "Another fruit", "") == -1);

    dict_free(&dict);
    printf("  [PASS] test_binary_search_and_crud\n");
}

static void test_prefix_and_substring_search(void) {
    Dictionary dict;
    dict_init(&dict);

    dict_add(&dict, "compiler", "noun", "Code translator", "C compiler");
    dict_add(&dict, "concurrency", "noun", "Simultaneous tasks", "Safe concurrency");
    dict_add(&dict, "constant", "noun", "Fixed value", "Math constant");
    dict_add(&dict, "pointer", "noun", "Memory address", "Null pointer");
    dict_add(&dict, "recursion", "noun", "Self-calling", "Stack recursion");

    /* Prefix search: "co" should match compiler, concurrency, constant */
    int results[10];
    int count = dict_prefix_search(&dict, "co", results, 10);
    assert(count == 3);
    assert(strcmp(dict.entries[results[0]].word, "compiler") == 0);
    assert(strcmp(dict.entries[results[1]].word, "concurrency") == 0);
    assert(strcmp(dict.entries[results[2]].word, "constant") == 0);

    /* Prefix not found */
    assert(dict_prefix_search(&dict, "xyz", results, 10) == 0);

    /* Substring search in meaning: "memory" matches pointer */
    count = dict_substring_search(&dict, "memory", results, 10);
    assert(count == 1);
    assert(strcmp(dict.entries[results[0]].word, "pointer") == 0);

    dict_free(&dict);
    printf("  [PASS] test_prefix_and_substring_search\n");
}

static void test_seed_loading_and_word_of_the_day(void) {
    Dictionary dict;
    dict_init(&dict);

    int loaded = dict_load_seed_file(&dict, "data/seed_words.txt");
    if (loaded == 0) {
        loaded = dict_load_seed_file(&dict, "15_dictionary_app/data/seed_words.txt");
    }
    assert(loaded >= 30);
    assert(dict.count >= 30);

    /* Verify binary search finds seeded words */
    assert(dict_binary_search(&dict, "algorithm") != NULL);
    assert(dict_binary_search(&dict, "validation") != NULL);
    assert(dict_binary_search(&dict, "sanitizer") != NULL);

    /* Word of the day returns a valid non-null entry */
    const DictEntry *wod = dict_get_word_of_the_day(&dict, 0);
    assert(wod != NULL);
    assert(!wod->is_deleted);
    assert(wod->word[0] != '\0');

    dict_free(&dict);
    printf("  [PASS] test_seed_loading_and_word_of_the_day\n");
}

static void test_persistence_and_file_integrity(void) {
    char templ[] = "/tmp/dict_test_XXXXXX";
    char *dir = mkdtemp(templ);
    assert(dir != NULL);
    setenv("DATA_DIR", dir, 1);

    Dictionary d1;
    dict_init(&d1);
    assert(dict_load(&d1));
    int base_count = d1.count;

    dict_add(&d1, "byte", "noun", "Eight bits", "One byte");
    dict_add(&d1, "word", "noun", "Vocabulary unit", "Spoken word");
    assert(dict_save(&d1));
    dict_free(&d1);

    /* Reload into d2 */
    Dictionary d2;
    dict_init(&d2);
    assert(dict_load(&d2));
    assert(d2.count == base_count + 2);
    assert(dict_binary_search(&d2, "byte") != NULL);
    assert(dict_binary_search(&d2, "word") != NULL);

    /* Save v2 creates .bak */
    dict_add(&d2, "bit", "noun", "Single binary digit", "Zero or one");
    assert(dict_save(&d2));

    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir, DICT_DATA_FILE);

    /* Corrupt file */
    FILE *fp = fopen(path, "wb");
    assert(fp != NULL);
    fputs("CORRUPT_DICT_DATA", fp);
    fclose(fp);

    /* Recover from backup */
    assert(safe_recover_backup(path, DICT_MAGIC, sizeof(DictEntry)));

    Dictionary d3;
    dict_init(&d3);
    assert(dict_load(&d3));
    assert(d3.count == base_count + 2); /* Restored to v1 */

    dict_free(&d2);
    dict_free(&d3);

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", dir);
    (void)system(cmd);

    printf("  [PASS] test_persistence_and_file_integrity\n");
}

int main(void) {
    printf("=== Running Dictionary App Unit Tests ===\n");
    test_binary_search_and_crud();
    test_prefix_and_substring_search();
    test_seed_loading_and_word_of_the_day();
    test_persistence_and_file_integrity();
    printf("All dictionary app unit tests passed successfully!\n");
    return 0;
}
