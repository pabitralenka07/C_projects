/*
 * To-Do List — Main Entry Point
 * Fast, persisted task manager with priorities, categories,
 * filters, and atomic crash-safe binary saves.
 */

#include "todo.h"
#include "input.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

static void print_menu(void) {
    printf("\n");
    util_print_header("To-Do List Manager");
    printf("  1. Add Task\n");
    printf("  2. List All Tasks\n");
    printf("  3. Toggle Task Done / Pending\n");
    printf("  4. Filter Tasks (Status / Priority)\n");
    printf("  5. Search Tasks\n");
    printf("  6. Update Task\n");
    printf("  7. Delete Task\n");
    printf("  8. Exit\n\n");
}

static void do_add_task(TodoList *list) {
    printf("\n--- Add Task ---\n");
    char title[TODO_TITLE_LEN];
    char category[TODO_CAT_LEN];
    int priority;

    if (!input_read_string("  Task Title: ", title, sizeof(title))) return;

    printf("  Category [e.g. Work, Study, Personal] (Enter for General): ");
    fflush(stdout);
    if (!input_read_line(category, sizeof(category), stdin)) return;
    input_trim(category);

    printf("  Priority (1 = Low, 2 = Medium, 3 = High) [default: 2]: ");
    fflush(stdout);
    char pbuf[16];
    if (!input_read_line(pbuf, sizeof(pbuf), stdin)) return;
    input_trim(pbuf);
    if (pbuf[0] == '1') priority = PRIORITY_LOW;
    else if (pbuf[0] == '3') priority = PRIORITY_HIGH;
    else priority = PRIORITY_MEDIUM;

    int id = todo_add(list, title, category, priority);
    if (id > 0) {
        printf("\n  Task added successfully! (ID: %d)\n", id);
        todo_save(list);
    }
}

static void do_toggle_task(TodoList *list) {
    int id;
    if (!input_read_int("  Enter Task ID to toggle: ", &id, 1, 999999)) return;

    TodoItem *item = todo_find_by_id(list, id);
    if (!item) {
        printf("  Task with ID %d not found.\n", id);
        return;
    }

    if (todo_toggle(list, id)) {
        printf("\n  Task #%d is now marked as %s!\n",
               id, item->is_done ? "[COMPLETED]" : "[PENDING]");
        todo_save(list);
    }
}

static void do_filter_tasks(TodoList *list) {
    printf("\n--- Filter Tasks ---\n");
    printf("  Status: 0 = All, 1 = Pending only, 2 = Completed only\n");
    int status_choice;
    if (!input_read_int("  Choose status filter: ", &status_choice, 0, 2)) return;

    printf("  Priority: 0 = All, 1 = Low, 2 = Medium, 3 = High\n");
    int prio_choice;
    if (!input_read_int("  Choose priority filter: ", &prio_choice, 0, 3)) return;

    todo_list_filtered(list, status_choice, prio_choice);
}

static void do_search_tasks(TodoList *list) {
    char query[64];
    if (!input_read_string("  Search query: ", query, sizeof(query))) return;

    int results[50];
    int found = todo_search(list, query, results, 50);
    if (found == 0) {
        printf("  No tasks found matching '%s'.\n", query);
        return;
    }

    printf("\n  Found %d match(es):\n", found);
    for (int i = 0; i < found; i++) {
        TodoItem *item = todo_find_by_id(list, results[i]);
        if (item) {
            printf("  [%d] %-6s [%s] (%s): %s\n",
                   item->id,
                   item->is_done ? "DONE" : "TODO",
                   todo_priority_str(item->priority),
                   item->category,
                   item->title);
        }
    }
}

static void do_update_task(TodoList *list) {
    int id;
    if (!input_read_int("  Enter Task ID to update: ", &id, 1, 999999)) return;

    TodoItem *item = todo_find_by_id(list, id);
    if (!item) {
        printf("  Task with ID %d not found.\n", id);
        return;
    }

    printf("\n  Current title: %s\n", item->title);
    printf("  New title (Enter to keep current): ");
    fflush(stdout);
    char title[TODO_TITLE_LEN];
    if (!input_read_line(title, sizeof(title), stdin)) return;
    input_trim(title);

    printf("  Current category: %s\n", item->category);
    printf("  New category (Enter to keep current): ");
    fflush(stdout);
    char category[TODO_CAT_LEN];
    if (!input_read_line(category, sizeof(category), stdin)) return;
    input_trim(category);

    printf("  Current priority: %s\n", todo_priority_str(item->priority));
    printf("  New priority (1=Low, 2=Med, 3=High, Enter to keep current): ");
    fflush(stdout);
    char pbuf[16];
    int priority = item->priority;
    if (input_read_line(pbuf, sizeof(pbuf), stdin)) {
        input_trim(pbuf);
        if (pbuf[0] == '1') priority = PRIORITY_LOW;
        else if (pbuf[0] == '2') priority = PRIORITY_MEDIUM;
        else if (pbuf[0] == '3') priority = PRIORITY_HIGH;
    }

    if (todo_update(list, id, title[0] ? title : NULL,
                    category[0] ? category : NULL, priority)) {
        printf("\n  Task updated successfully!\n");
        todo_save(list);
    }
}

static void do_delete_task(TodoList *list) {
    int id;
    if (!input_read_int("  Enter Task ID to delete: ", &id, 1, 999999)) return;

    TodoItem *item = todo_find_by_id(list, id);
    if (!item) {
        printf("  Task with ID %d not found.\n", id);
        return;
    }

    if (input_confirm("  Are you sure you want to delete this task?", false)) {
        if (todo_delete(list, id)) {
            printf("  Task deleted.\n");
            todo_save(list);
        }
    } else {
        printf("  Deletion cancelled.\n");
    }
}

int main(void) {
    TodoList list;
    todo_init(&list);

    printf("Loading to-do items...\n");
    todo_load(&list);
    printf("Ready.\n");

    for (;;) {
        print_menu();
        int choice = input_menu_choice(1, 8);
        if (choice == -1 || choice == 8) {
            break;
        }

        switch (choice) {
        case 1:
            do_add_task(&list);
            break;
        case 2:
            todo_list_all(&list);
            break;
        case 3:
            do_toggle_task(&list);
            break;
        case 4:
            do_filter_tasks(&list);
            break;
        case 5:
            do_search_tasks(&list);
            break;
        case 6:
            do_update_task(&list);
            break;
        case 7:
            do_delete_task(&list);
            break;
        default:
            break;
        }

        util_press_enter();
    }

    printf("\nSaving tasks...\n");
    todo_save(&list);
    todo_free(&list);
    printf("Goodbye!\n");
    return EXIT_SUCCESS;
}
