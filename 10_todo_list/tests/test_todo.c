#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "todo.h"

static void make_temp_dir(char *buf, size_t size) {
    char templ[] = "/tmp/todo_test_XXXXXX";
    char *dir = mkdtemp(templ);
    assert(dir != NULL);
    snprintf(buf, size, "%s", dir);
}

static void cleanup_temp_dir(const char *dir) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", dir);
    (void)system(cmd);
}

static void test_crud_and_toggle(void) {
    TodoList list;
    todo_init(&list);

    int id1 = todo_add(&list, "Write C unit tests", "Dev", PRIORITY_HIGH);
    int id2 = todo_add(&list, "Buy groceries", "Personal", PRIORITY_LOW);
    assert(id1 == 1);
    assert(id2 == 2);
    assert(list.count == 2);

    TodoItem *item1 = todo_find_by_id(&list, 1);
    assert(item1 != NULL);
    assert(strcmp(item1->title, "Write C unit tests") == 0);
    assert(item1->priority == PRIORITY_HIGH);
    assert(item1->is_done == 0);

    /* Toggle completion */
    assert(todo_toggle(&list, 1));
    assert(item1->is_done == 1);
    assert(todo_toggle(&list, 1));
    assert(item1->is_done == 0);

    /* Update */
    assert(todo_update(&list, 2, "Buy milk and bread", "Home", PRIORITY_MEDIUM));
    TodoItem *item2 = todo_find_by_id(&list, 2);
    assert(strcmp(item2->title, "Buy milk and bread") == 0);
    assert(strcmp(item2->category, "Home") == 0);
    assert(item2->priority == PRIORITY_MEDIUM);

    /* Delete */
    assert(todo_delete(&list, 1));
    assert(todo_find_by_id(&list, 1) == NULL);

    todo_free(&list);
    printf("  [PASS] test_crud_and_toggle\n");
}

static void test_search(void) {
    TodoList list;
    todo_init(&list);

    todo_add(&list, "Refactor database engine", "Backend", PRIORITY_HIGH);
    todo_add(&list, "Design frontend dashboard", "Frontend", PRIORITY_MEDIUM);
    todo_add(&list, "Fix database index leak", "Backend", PRIORITY_HIGH);

    int results[10];
    int count = todo_search(&list, "database", results, 10);
    assert(count == 2);
    assert(results[0] == 1);
    assert(results[1] == 3);

    count = todo_search(&list, "Frontend", results, 10);
    assert(count == 1);
    assert(results[0] == 2);

    todo_free(&list);
    printf("  [PASS] test_search\n");
}

static void test_capacity_growth(void) {
    TodoList list;
    todo_init(&list);

    for (int i = 0; i < 50; i++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Task number %d", i + 1);
        int id = todo_add(&list, buf, "Stress", PRIORITY_LOW);
        assert(id == i + 1);
    }
    assert(list.count == 50);
    assert(list.capacity >= 50);

    todo_free(&list);
    printf("  [PASS] test_capacity_growth\n");
}

static void test_persistence_roundtrip(void) {
    char test_dir[256];
    make_temp_dir(test_dir, sizeof(test_dir));
    setenv("DATA_DIR", test_dir, 1);

    TodoList l1;
    todo_init(&l1);
    todo_load(&l1);

    todo_add(&l1, "Persistent Task 1", "Work", PRIORITY_HIGH);
    todo_add(&l1, "Persistent Task 2", "Play", PRIORITY_LOW);
    todo_toggle(&l1, 1);
    assert(todo_save(&l1));
    todo_free(&l1);

    TodoList l2;
    todo_init(&l2);
    assert(todo_load(&l2));
    assert(l2.count == 2);
    assert(l2.next_id == 3);
    assert(l2.items[0].is_done == 1);
    assert(strcmp(l2.items[0].title, "Persistent Task 1") == 0);
    assert(l2.items[1].is_done == 0);
    assert(strcmp(l2.items[1].title, "Persistent Task 2") == 0);

    todo_free(&l2);
    cleanup_temp_dir(test_dir);
    printf("  [PASS] test_persistence_roundtrip\n");
}

int main(void) {
    printf("=== Running To-Do List Unit Tests ===\n");
    test_crud_and_toggle();
    test_search();
    test_capacity_growth();
    test_persistence_roundtrip();
    printf("All to-do list unit tests passed!\n");
    return 0;
}
