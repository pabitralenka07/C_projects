#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "airport.h"

static void make_temp_dir(char *buf, size_t size) {
    char templ[] = "/tmp/air_test_XXXXXX";
    char *dir = mkdtemp(templ);
    assert(dir != NULL);
    snprintf(buf, size, "%s", dir);
}

static void cleanup_temp_dir(const char *dir) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", dir);
    (void)system(cmd);
}

static void test_flight_crud(void) {
    Airport air;
    airport_init(&air);

    int f1 = flight_add(&air, "UA-881", "United", "Chicago", "Tokyo", "12:00", "15:30", 5, 250);
    int f2 = flight_add(&air, "AF-006", "Air France", "Paris", "New York", "14:15", "16:45", 12, 280);
    assert(f1 == 1 && f2 == 2);
    assert(air.flight_count == 2);

    Flight *f = flight_find_by_number(&air, "ua-881");
    assert(f != NULL && f->id == 1);
    assert(strcmp(f->destination, "Tokyo") == 0);

    /* Update status and gate */
    assert(flight_update_status(&air, 1, "Boarding"));
    assert(strcmp(f->status, "Boarding") == 0);
    assert(flight_update_gate(&air, 1, 9));
    assert(f->gate == 9);

    /* Delete */
    assert(flight_delete(&air, 2));
    assert(flight_find_by_id(&air, 2) == NULL);

    airport_free(&air);
    printf("  [PASS] test_flight_crud\n");
}

static void test_passenger_booking_and_checkin(void) {
    Airport air;
    airport_init(&air);

    int fid = flight_add(&air, "BA-212", "British Airways", "Boston", "London", "19:00", "06:30", 7, 2);
    Flight *f = flight_find_by_id(&air, fid);
    assert(f != NULL);

    /* Book passenger 1 */
    int p1 = passenger_book(&air, "Alan Turing", "UK-1912", fid);
    assert(p1 == 1);
    Passenger *pass1 = passenger_find_by_id(&air, p1);
    assert(pass1 != NULL && pass1->seat_number == 1 && !pass1->checked_in);

    /* Book passenger 2 */
    int p2 = passenger_book(&air, "Joan Clarke", "UK-1917", fid);
    assert(p2 == 2);
    Passenger *pass2 = passenger_find_by_id(&air, p2);
    assert(pass2 != NULL && pass2->seat_number == 2);

    /* Book passenger 3 (should fail due to capacity 2) */
    assert(passenger_book(&air, "Overbooked Person", "XX-9999", fid) == -1);

    /* Check-in */
    assert(passenger_checkin(&air, p1));
    assert(pass1->checked_in == 1);
    assert(f->checked_in == 1);

    /* Already checked-in check */
    assert(!passenger_checkin(&air, p1));

    airport_free(&air);
    printf("  [PASS] test_passenger_booking_and_checkin\n");
}

static void test_flight_search(void) {
    Airport air;
    airport_init(&air);

    flight_add(&air, "QF-011", "Qantas", "Sydney", "Los Angeles", "11:00", "07:00", 20, 300);
    flight_add(&air, "EK-201", "Emirates", "Dubai", "New York", "08:30", "14:15", 3, 400);

    int results[10];
    int count = flight_search(&air, "Dubai", results, 10);
    assert(count == 1 && results[0] == 2);

    count = flight_search(&air, "Qantas", results, 10);
    assert(count == 1 && results[0] == 1);

    airport_free(&air);
    printf("  [PASS] test_flight_search\n");
}

static void test_persistence_roundtrip(void) {
    char test_dir[256];
    make_temp_dir(test_dir, sizeof(test_dir));
    setenv("DATA_DIR", test_dir, 1);

    Airport a1;
    airport_init(&a1);
    airport_load(&a1);

    int fid = flight_add(&a1, "SQ-022", "Singapore Airlines", "Singapore", "Newark", "23:35", "06:00", 15, 160);
    int pid = passenger_book(&a1, "John von Neumann", "US-1903", fid);
    passenger_checkin(&a1, pid);
    assert(airport_save(&a1));
    airport_free(&a1);

    Airport a2;
    airport_init(&a2);
    assert(airport_load(&a2));
    assert(a2.flight_count == 1);
    assert(a2.passenger_count == 1);
    assert(strcmp(a2.flights[0].flight_number, "SQ-022") == 0);
    assert(a2.flights[0].checked_in == 1);
    assert(a2.passengers[0].checked_in == 1);
    assert(strcmp(a2.passengers[0].name, "John von Neumann") == 0);

    airport_free(&a2);
    cleanup_temp_dir(test_dir);
    printf("  [PASS] test_persistence_roundtrip\n");
}

int main(void) {
    printf("=== Running Airport Management Unit Tests ===\n");
    test_flight_crud();
    test_passenger_booking_and_checkin();
    test_flight_search();
    test_persistence_roundtrip();
    printf("All airport unit tests passed!\n");
    return 0;
}
