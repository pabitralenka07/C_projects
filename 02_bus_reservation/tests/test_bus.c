#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bus.h"

static void make_temp_dir(char *buf, size_t size) {
    char templ[] = "/tmp/bus_test_XXXXXX";
    char *dir = mkdtemp(templ);
    assert(dir != NULL);
    snprintf(buf, size, "%s", dir);
}

static void cleanup_temp_dir(const char *dir) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", dir);
    (void)system(cmd);
}

static void test_bus_crud(void) {
    BusSystem sys;
    bus_system_init(&sys);

    int b1 = bus_add(&sys, "Intercity Flyer", "Seattle", "Portland", "08:00", 25.0, 32);
    int b2 = bus_add(&sys, "Coastal Liner", "San Francisco", "San Jose", "10:30", 15.0, 40);
    assert(b1 == 1 && b2 == 2);
    assert(sys.bus_count == 2);

    Bus *bus = bus_find_by_id(&sys, 1);
    assert(bus != NULL);
    assert(strcmp(bus->name, "Intercity Flyer") == 0);
    assert(bus_available_seats_count(bus) == 32);

    /* Search */
    int res[10];
    int count = bus_search_route(&sys, "Seattle", "Portland", res, 10);
    assert(count == 1 && res[0] == 1);

    /* Delete */
    assert(bus_delete(&sys, 2));
    assert(bus_find_by_id(&sys, 2) == NULL);

    bus_system_free(&sys);
    printf("  [PASS] test_bus_crud\n");
}

static void test_seat_booking_and_cancellation(void) {
    BusSystem sys;
    bus_system_init(&sys);

    int bid = bus_add(&sys, "Metro Link", "Austin", "Houston", "09:00", 30.0, 20);
    Bus *b = bus_find_by_id(&sys, bid);
    assert(b != NULL);

    /* Book seat 5 */
    int bk1 = booking_create(&sys, bid, "Linus Torvalds", "555-0100", 5);
    assert(bk1 == 1);
    assert(b->seat_map[4] == 1); /* 0-indexed */
    assert(bus_available_seats_count(b) == 19);

    /* Attempt to double-book seat 5 (should be rejected) */
    int bk_dup = booking_create(&sys, bid, "Another Passenger", "555-0101", 5);
    assert(bk_dup == -1);

    /* Out of range seat */
    assert(booking_create(&sys, bid, "Bad Seat", "555-0000", 99) == -1);

    /* Cancel booking */
    assert(booking_cancel(&sys, bk1));
    assert(b->seat_map[4] == 0); /* Seat 5 released */
    assert(bus_available_seats_count(b) == 20);

    bus_system_free(&sys);
    printf("  [PASS] test_seat_booking_and_cancellation\n");
}

static void test_persistence_roundtrip(void) {
    char test_dir[256];
    make_temp_dir(test_dir, sizeof(test_dir));
    setenv("DATA_DIR", test_dir, 1);

    BusSystem s1;
    bus_system_init(&s1);
    bus_system_load(&s1);

    int bid = bus_add(&s1, "Night Express", "Denver", "Salt Lake City", "22:00", 55.0, 40);
    int bkid = booking_create(&s1, bid, "Ada Lovelace", "555-1843", 12);
    assert(bkid == 1);
    assert(bus_system_save(&s1));
    bus_system_free(&s1);

    BusSystem s2;
    bus_system_init(&s2);
    assert(bus_system_load(&s2));
    assert(s2.bus_count == 1);
    assert(s2.booking_count == 1);
    assert(s2.buses[0].seat_map[11] == 1); /* Seat 12 is booked */
    assert(strcmp(s2.bookings[0].passenger_name, "Ada Lovelace") == 0);

    bus_system_free(&s2);
    cleanup_temp_dir(test_dir);
    printf("  [PASS] test_persistence_roundtrip\n");
}

int main(void) {
    printf("=== Running Bus Reservation Unit Tests ===\n");
    test_bus_crud();
    test_seat_booking_and_cancellation();
    test_persistence_roundtrip();
    printf("All bus reservation unit tests passed!\n");
    return 0;
}
