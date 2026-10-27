#ifndef TODO_H
#define TODO_H

#include <stdbool.h>
#include <stddef.h>

#define TODO_TITLE_LEN    128
#define TODO_CAT_LEN      32
#define TODO_MAGIC        "TDOi"
#define TODO_DATA_FILE    "todo.dat"

typedef enum {
    PRIORITY_LOW = 1,
    PRIORITY_MEDIUM = 2,
    PRIORITY_HIGH = 3
} Priority;

typedef struct {
    int  id;
    char title[TODO_TITLE_LEN];
    char category[TODO_CAT_LEN];
    int  priority;       /* 1=Low, 2=Medium, 3=High */
    int  is_done;        /* 0=Pending, 1=Completed */
    int  is_deleted;     /* 0=Active, 1=Soft deleted */
} TodoItem;

typedef struct {
    TodoItem *items;
    int       count;
    int       capacity;
    int       next_id;
    char      data_dir[256];
} TodoList;

/* Lifecycle */
void todo_init(TodoList *list);
void todo_free(TodoList *list);
bool todo_load(TodoList *list);
bool todo_save(TodoList *list);

/* Operations */
int  todo_add(TodoList *list, const char *title, const char *category, int priority);
bool todo_toggle(TodoList *list, int id);
bool todo_update(TodoList *list, int id, const char *new_title, const char *new_cat, int new_priority);
bool todo_delete(TodoList *list, int id);
TodoItem *todo_find_by_id(TodoList *list, int id);

/* Search and display */
void todo_list_all(const TodoList *list);
void todo_list_filtered(const TodoList *list, int filter_status, int filter_priority);
int  todo_search(const TodoList *list, const char *query, int *results, int max_results);
bool todo_validate(const TodoItem *item);
const char *todo_priority_str(int priority);

/* Menu */
void todo_menu(TodoList *list);

#endif /* TODO_H */
