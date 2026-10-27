#ifndef DICT_H
#define DICT_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define DICT_WORD_LEN    48
#define DICT_POS_LEN     16
#define DICT_MEAN_LEN    256
#define DICT_EX_LEN      256

#define DICT_MAGIC       "DICT"
#define DICT_DATA_FILE   "dictionary.dat"
#define DICT_SEED_FILE   "seed_words.txt"

typedef struct {
    int  id;
    char word[DICT_WORD_LEN];           /* Lowercase for sorted indexing */
    char part_of_speech[DICT_POS_LEN];  /* "noun", "verb", "adjective", etc. */
    char meaning[DICT_MEAN_LEN];
    char example[DICT_EX_LEN];
    int  is_deleted;
} DictEntry;

typedef struct {
    DictEntry *entries;
    int        count;
    int        capacity;
    int        next_id;
    char       data_dir[256];
} Dictionary;

/* Lifecycle and Persistence */
void dict_init(Dictionary *dict);
void dict_free(Dictionary *dict);
bool dict_load(Dictionary *dict);
bool dict_save(Dictionary *dict);
int  dict_load_seed_file(Dictionary *dict, const char *filepath);

/* CRUD Operations (Maintains Sorted Order) */
int  dict_add(Dictionary *dict, const char *word, const char *pos,
              const char *meaning, const char *example);
bool dict_update(Dictionary *dict, const char *word, const char *pos,
                 const char *meaning, const char *example);
bool dict_delete(Dictionary *dict, const char *word);

/* Search Algorithms */
DictEntry *dict_binary_search(const Dictionary *dict, const char *word);
int dict_prefix_search(const Dictionary *dict, const char *prefix, int *out_indices, int max_results);
int dict_substring_search(const Dictionary *dict, const char *substring, int *out_indices, int max_results);

/* Word of the Day */
const DictEntry *dict_get_word_of_the_day(const Dictionary *dict, int seed_day_offset);

/* Display */
void dict_print_entry(const DictEntry *entry);
void dict_list_all(const Dictionary *dict);

#endif /* DICT_H */
