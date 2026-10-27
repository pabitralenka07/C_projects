#include "todo.h"
#include "file_ops.h"
#include "input.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void todo_init(TodoList *list) {
    if (!list) return;
    memset(list, 0, sizeof(TodoList));
    list->next_id = 1;

    const char *env = getenv("DATA_DIR");
    if (env && env[0] != '\0') {
        snprintf(list->data_dir, sizeof(list->data_dir), "%s", env);
    } else {
        snprintf(list->data_dir, sizeof(list->data_dir), "./data");
    }
}

void todo_free(TodoList *list) {
    if (!list) return;
    free(list->items);
    memset(list, 0, sizeof(TodoList));
}

static bool ensure_capacity(TodoList *list) {
    if (list->count < list->capacity) return true;

    int new_cap = (list->capacity == 0) ? 16 : list->capacity * 2;
    TodoItem *grown = realloc(list->items, (size_t)new_cap * sizeof(TodoItem));
    if (!grown) {
        fprintf(stderr, "Error: Memory allocation failed for todo items.\n");
        return false;
    }
    list->items = grown;
    list->capacity = new_cap;
    return true;
}

const char *todo_priority_str(int priority) {
    switch (priority) {
    case PRIORITY_HIGH:   return "HIGH 🔴";
    case PRIORITY_MEDIUM: return "MED  🟡";
    case PRIORITY_LOW:    return "LOW  🟢";
    default:              return "NORM   ";
    }
}

bool todo_validate(const TodoItem *item) {
    if (!item) return false;
    if (item->id <= 0) return false;
    if (item->title[0] == '\0') return false;
    if (item->title[TODO_TITLE_LEN - 1] != '\0') return false;
    if (item->category[TODO_CAT_LEN - 1] != '\0') return false;
    if (item->priority < 1 || item->priority > 3) return false;
    return true;
}

bool todo_load(TodoList *list) {
    if (!list) return false;
    if (!util_ensure_dir(list->data_dir)) {
        fprintf(stderr, "Error: Cannot create data directory '%s'.\n", list->data_dir);
        return false;
    }

    char path[512];
    snprintf(path, sizeof(path), "%s/%s", list->data_dir, TODO_DATA_FILE);

    void *data = NULL;
    uint32_t count = 0, next_id = 1;
    int res = file_validated_load(path, TODO_MAGIC, &data, sizeof(TodoItem), &count, &next_id);

    if (res == FILE_LOAD_CORRUPT) {
        printf("  WARNING: Todo data file corrupted!\n");
        if (input_confirm("  Attempt recovery from backup?", true)) {
            if (file_recover_backup(path)) {
                res = file_validated_load(path, TODO_MAGIC, &data, sizeof(TodoItem), &count, &next_id);
            }
        }
        if (res != FILE_LOAD_OK) {
            data = NULL; count = 0; next_id = 1;
        }
    }

    list->items    = (TodoItem *)data;
    list->count    = (int)count;
    list->capacity = (int)count;
    list->next_id  = (int)next_id;

    /* Validate items */
    for (int i = 0; i < list->count; i++) {
        if (!todo_validate(&list->items[i])) {
            list->items[i].is_deleted = 1;
        }
    }
    return true;
}

bool todo_save(TodoList *list) {
    if (!list) return false;
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", list->data_dir, TODO_DATA_FILE);

    return file_atomic_save(path, TODO_MAGIC, list->items, sizeof(TodoItem),
                            (uint32_t)list->count, (uint32_t)list->next_id);
}

int todo_add(TodoList *list, const char *title, const char *category, int priority) {
    if (!list || !title || title[0] == '\0') return -1;
    if (!ensure_capacity(list)) return -1;

    TodoItem *item = &list->items[list->count++];
    memset(item, 0, sizeof(TodoItem));
    item->id = list->next_id++;
    strncpy(item->title, title, sizeof(item->title) - 1);
    if (category && category[0] != '\0') {
        strncpy(item->category, category, sizeof(item->category) - 1);
    } else {
        strncpy(item->category, "General", sizeof(item->category) - 1);
    }
    item->priority   = (priority >= 1 && priority <= 3) ? priority : PRIORITY_MEDIUM;
    item->is_done    = 0;
    item->is_deleted = 0;

    return item->id;
}

TodoItem *todo_find_by_id(TodoList *list, int id) {
    if (!list || id <= 0) return NULL;
    for (int i = 0; i < list->count; i++) {
        if (list->items[i].id == id && !list->items[i].is_deleted) {
            return &list->items[i];
        }
    }
    return NULL;
}

bool todo_toggle(TodoList *list, int id) {
    TodoItem *item = todo_find_by_id(list, id);
    if (!item) return false;
    item->is_done = !item->is_done;
    return true;
}

bool todo_update(TodoList *list, int id, const char *new_title, const char *new_cat, int new_priority) {
    TodoItem *item = todo_find_by_id(list, id);
    if (!item) return false;

    if (new_title && new_title[0] != '\0') {
        strncpy(item->title, new_title, sizeof(item->title) - 1);
    }
    if (new_cat && new_cat[0] != '\0') {
        strncpy(item->category, new_cat, sizeof(item->category) - 1);
    }
    if (new_priority >= 1 && new_priority <= 3) {
        item->priority = new_priority;
    }
    return true;
}

bool todo_delete(TodoList *list, int id) {
    TodoItem *item = todo_find_by_id(list, id);
    if (!item) return false;
    item->is_deleted = 1;
    return true;
}

static bool str_contains_ci(const char *haystack, const char *needle) {
    size_t hlen = strlen(haystack), nlen = strlen(needle);
    if (nlen == 0) return true;
    if (nlen > hlen) return false;
    for (size_t i = 0; i <= hlen - nlen; i++) {
        bool match = true;
        for (size_t j = 0; j < nlen; j++) {
            if (tolower((unsigned char)haystack[i + j]) != tolower((unsigned char)needle[j])) {
                match = false;
                break;
            }
        }
        if (match) return true;
    }
    return false;
}

int todo_search(const TodoList *list, const char *query, int *results, int max_results) {
    if (!list || !query || !results || max_results <= 0) return 0;
    int found = 0;
    for (int i = 0; i < list->count && found < max_results; i++) {
        const TodoItem *item = &list->items[i];
        if (item->is_deleted) continue;
        if (str_contains_ci(item->title, query) || str_contains_ci(item->category, query)) {
            results[found++] = item->id;
        }
    }
    return found;
}

void todo_list_filtered(const TodoList *list, int filter_status, int filter_priority) {
    if (!list) return;

    printf("\n");
    util_print_separator(74);
    printf("  %-4s | %-6s | %-8s | %-12s | %s\n",
           "ID", "Status", "Priority", "Category", "Task Title");
    util_print_separator(74);

    int count = 0;
    for (int i = 0; i < list->count; i++) {
        const TodoItem *item = &list->items[i];
        if (item->is_deleted) continue;

        /* Filter by status: 1=Pending, 2=Completed, 0=All */
        if (filter_status == 1 && item->is_done) continue;
        if (filter_status == 2 && !item->is_done) continue;

        /* Filter by priority: 1, 2, 3 or 0=All */
        if (filter_priority > 0 && item->priority != filter_priority) continue;

        printf("  %-4d | %-6s | %-8s | %-12.12s | %s\n",
               item->id,
               item->is_done ? "[DONE]" : "[    ]",
               todo_priority_str(item->priority),
               item->category,
               item->title);
        count++;
    }

    util_print_separator(74);
    printf("  Total: %d task(s)\n", count);
}

void todo_list_all(const TodoList *list) {
    todo_list_filtered(list, 0, 0);
}
