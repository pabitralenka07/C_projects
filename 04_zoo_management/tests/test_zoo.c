#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "zoo.h"

static void make_temp_dir(char *buf, size_t size) {
    char templ[] = "/tmp/zoo_test_XXXXXX";
    char *dir = mkdtemp(templ);
    assert(dir != NULL);
    snprintf(buf, size, "%s", dir);
}

static void cleanup_temp_dir(const char *dir) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", dir);
    (void)system(cmd);
}

static void test_enclosures_and_animals(void) {
    Zoo z;
    zoo_init(&z);

    int e1 = enclosure_add(&z, "Elephant Sanctuary", "Open Pasture", 2);
    assert(e1 == 1);
    Enclosure *enc = enclosure_find_by_id(&z, 1);
    assert(enc != NULL && enc->current_count == 0);

    /* Add animal 1 */
    int a1 = animal_add(&z, "Dumbo", "Asian Elephant", 10, e1, "08:00 Hay 50kg");
    assert(a1 == 1 && enc->current_count == 1);

    /* Add animal 2 */
    int a2 = animal_add(&z, "Manny", "Mammoth", 15, e1, "12:00 Grass 60kg");
    assert(a2 == 2 && enc->current_count == 2);

    /* Add animal 3 (should fail due to enclosure capacity 2) */
    assert(animal_add(&z, "Extra", "Elephant", 4, e1, "Hay") == -1);

    /* Search */
    int res[10];
    int count = animal_search(&z, "Mammoth", res, 10);
    assert(count == 1 && res[0] == 2);

    zoo_free(&z);
    printf("  [PASS] test_enclosures_and_animals\n");
}

static void test_animal_transfer(void) {
    Zoo z;
    zoo_init(&z);

    int e1 = enclosure_add(&z, "Habitat Alpha", "Plains", 5);
    int e2 = enclosure_add(&z, "Habitat Beta", "Forest", 5);

    int a1 = animal_add(&z, "George", "Gorilla", 12, e1, "Fruit 10kg");
    Enclosure *enc1 = enclosure_find_by_id(&z, e1);
    Enclosure *enc2 = enclosure_find_by_id(&z, e2);
    assert(enc1->current_count == 1 && enc2->current_count == 0);

    /* Transfer to e2 */
    assert(animal_transfer(&z, a1, e2));
    assert(enc1->current_count == 0);
    assert(enc2->current_count == 1);

    Animal *a = animal_find_by_id(&z, a1);
    assert(a->enclosure_id == e2);

    zoo_free(&z);
    printf("  [PASS] test_animal_transfer\n");
}

static void test_staff_and_ticketing(void) {
    Zoo z;
    zoo_init(&z);

    int e1 = enclosure_add(&z, "Penguin Cove", "Aquatic", 20);
    int s1 = staff_add(&z, "David Attenborough", "Senior Biologist", "555-0199", e1);
    assert(s1 == 1);

    Staff *st = staff_find_by_id(&z, 1);
    assert(st != NULL && strcmp(st->name, "David Attenborough") == 0);

    /* Ticketing: 2 adults ($20 ea) + 3 children ($12 ea) = 40 + 36 = $76 */
    int t1 = ticket_sell(&z, 2, 1);
    int t2 = ticket_sell(&z, 3, 2);
    assert(t1 == 1 && t2 == 2);
    assert(ticket_total_revenue(&z) == 76.0);

    zoo_free(&z);
    printf("  [PASS] test_staff_and_ticketing\n");
}

static void test_persistence_roundtrip(void) {
    char test_dir[256];
    make_temp_dir(test_dir, sizeof(test_dir));
    setenv("DATA_DIR", test_dir, 1);

    Zoo z1;
    zoo_init(&z1);
    zoo_load(&z1);

    int eid = enclosure_add(&z1, "Tiger Valley", "Open Jungle", 4);
    int aid = animal_add(&z1, "Shere Khan", "Bengal Tiger", 8, eid, "Raw meat 8kg");
    assert(aid > 0);
    staff_add(&z1, "Keeper Mowgli", "Animal Handler", "555-1894", eid);
    ticket_sell(&z1, 5, 1); /* 5 * 20 = $100 */

    assert(zoo_save(&z1));
    zoo_free(&z1);

    Zoo z2;
    zoo_init(&z2);
    assert(zoo_load(&z2));
    assert(z2.enclosure_count == 1);
    assert(z2.animal_count == 1);
    assert(z2.staff_count == 1);
    assert(z2.ticket_count == 1);
    assert(strcmp(z2.animals[0].name, "Shere Khan") == 0);
    assert(z2.enclosures[0].current_count == 1);
    assert(ticket_total_revenue(&z2) == 100.0);

    zoo_free(&z2);
    cleanup_temp_dir(test_dir);
    printf("  [PASS] test_persistence_roundtrip\n");
}

int main(void) {
    printf("=== Running Zoo Management Unit Tests ===\n");
    test_enclosures_and_animals();
    test_animal_transfer();
    test_staff_and_ticketing();
    test_persistence_roundtrip();
    printf("All zoo management unit tests passed!\n");
    return 0;
}
