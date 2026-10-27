#include "bus.h"
#include "file_ops.h"
#include "input.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define FILE_BUSES "buses.dat"
#define FILE_BOOKS "bookings.dat"

void bus_system_init(BusSystem *sys) {
    if (!sys) return;
    memset(sys, 0, sizeof(BusSystem));
    sys->next_bus_id     = 1;
    sys->next_booking_id = 1;

    const char *env = getenv("DATA_DIR");
    if (env && env[0] != '\0') {
        snprintf(sys->data_dir, sizeof(sys->data_dir), "%s", env);
    } else {
        snprintf(sys->data_dir, sizeof(sys->data_dir), "./data");
    }
}

void bus_system_free(BusSystem *sys) {
    if (!sys) return;
    free(sys->buses);
    free(sys->bookings);
    memset(sys, 0, sizeof(BusSystem));
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

static bool load_records(const char *dir, const char *filename,
                         const char *magic, void **out_data,
                         uint32_t *out_count, uint32_t *out_next_id,
                         size_t record_size) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir, filename);

    void *data = NULL;
    uint32_t count = 0, next_id = 1;
    int res = file_validated_load(path, magic, &data, record_size, &count, &next_id);

    if (res == FILE_LOAD_CORRUPT) {
        printf("  WARNING: %s is corrupt. Restoring from backup...\n", filename);
        if (file_recover_backup(path)) {
            res = file_validated_load(path, magic, &data, record_size, &count, &next_id);
        }
        if (res != FILE_LOAD_OK) {
            data = NULL; count = 0; next_id = 1;
        }
    }

    *out_data    = data;
    *out_count   = count;
    *out_next_id = next_id;
    return true;
}

bool bus_system_load(BusSystem *sys) {
    if (!sys) return false;
    if (!util_ensure_dir(sys->data_dir)) return false;

    void *data;
    uint32_t count, next_id;

    /* Buses */
    load_records(sys->data_dir, FILE_BUSES, BUS_MAGIC_BUS, &data, &count, &next_id, sizeof(Bus));
    sys->buses        = (Bus *)data;
    sys->bus_count    = (int)count;
    sys->bus_capacity = (int)count;
    sys->next_bus_id  = (int)next_id;

    /* Bookings */
    load_records(sys->data_dir, FILE_BOOKS, BUS_MAGIC_BOOK, &data, &count, &next_id, sizeof(Booking));
    sys->bookings         = (Booking *)data;
    sys->booking_count    = (int)count;
    sys->booking_capacity = (int)count;
    sys->next_booking_id  = (int)next_id;

    return true;
}

bool bus_system_save(BusSystem *sys) {
    if (!sys) return false;
    char path[512];

    snprintf(path, sizeof(path), "%s/%s", sys->data_dir, FILE_BUSES);
    file_atomic_save(path, BUS_MAGIC_BUS, sys->buses, sizeof(Bus),
                     (uint32_t)sys->bus_count, (uint32_t)sys->next_bus_id);

    snprintf(path, sizeof(path), "%s/%s", sys->data_dir, FILE_BOOKS);
    file_atomic_save(path, BUS_MAGIC_BOOK, sys->bookings, sizeof(Booking),
                     (uint32_t)sys->booking_count, (uint32_t)sys->next_booking_id);

    return true;
}

/* ------------------------------------------------------------------ */
/* Bus Operations                                                     */
/* ------------------------------------------------------------------ */

int bus_add(BusSystem *sys, const char *name, const char *from, const char *to,
            const char *time_str, double fare, int total_seats) {
    if (!sys || !name || !from || !to) return -1;
    if (total_seats <= 0 || total_seats > BUS_MAX_SEATS) total_seats = 40;

    if (sys->bus_count >= sys->bus_capacity) {
        int new_cap = (sys->bus_capacity == 0) ? 8 : sys->bus_capacity * 2;
        Bus *grown = realloc(sys->buses, (size_t)new_cap * sizeof(Bus));
        if (!grown) return -1;
        sys->buses = grown;
        sys->bus_capacity = new_cap;
    }

    Bus *b = &sys->buses[sys->bus_count++];
    memset(b, 0, sizeof(Bus));
    b->id = sys->next_bus_id++;
    strncpy(b->name, name, sizeof(b->name) - 1);
    strncpy(b->route_from, from, sizeof(b->route_from) - 1);
    strncpy(b->route_to, to, sizeof(b->route_to) - 1);
    if (time_str) strncpy(b->departure_time, time_str, sizeof(b->departure_time) - 1);
    else strncpy(b->departure_time, "08:00", sizeof(b->departure_time) - 1);
    b->fare = fare > 0.0 ? fare : 25.0;
    b->total_seats = total_seats;
    b->is_deleted = 0;

    return b->id;
}

Bus *bus_find_by_id(BusSystem *sys, int id) {
    if (!sys || id <= 0) return NULL;
    for (int i = 0; i < sys->bus_count; i++) {
        if (sys->buses[i].id == id && !sys->buses[i].is_deleted) {
            return &sys->buses[i];
        }
    }
    return NULL;
}

bool bus_delete(BusSystem *sys, int id) {
    Bus *b = bus_find_by_id(sys, id);
    if (!b) return false;
    b->is_deleted = 1;
    return true;
}

int bus_available_seats_count(const Bus *bus) {
    if (!bus) return 0;
    int count = 0;
    for (int i = 0; i < bus->total_seats; i++) {
        if (bus->seat_map[i] == 0) count++;
    }
    return count;
}

void bus_list_all(const BusSystem *sys) {
    if (!sys) return;
    printf("\n");
    util_print_separator(76);
    printf("  %-4s | %-20s | %-20s | %-6s | %-7s | %s\n",
           "ID", "Bus Name / Operator", "Route", "Time", "Fare", "Available Seats");
    util_print_separator(76);

    int count = 0;
    for (int i = 0; i < sys->bus_count; i++) {
        const Bus *b = &sys->buses[i];
        if (b->is_deleted) continue;
        char route[64];
        snprintf(route, sizeof(route), "%s -> %s", b->route_from, b->route_to);

        printf("  %-4d | %-20.20s | %-20.20s | %-6s | $%-6.2f | %d / %d\n",
               b->id, b->name, route, b->departure_time, b->fare,
               bus_available_seats_count(b), b->total_seats);
        count++;
    }
    util_print_separator(76);
    printf("  Total: %d bus(es) active\n", count);
}

int bus_search_route(const BusSystem *sys, const char *from, const char *to,
                     int *results, int max_results) {
    if (!sys || !results || max_results <= 0) return 0;
    int found = 0;
    for (int i = 0; i < sys->bus_count && found < max_results; i++) {
        const Bus *b = &sys->buses[i];
        if (b->is_deleted) continue;

        bool match_from = (!from || from[0] == '\0' || str_contains_ci(b->route_from, from));
        bool match_to   = (!to   || to[0] == '\0'   || str_contains_ci(b->route_to, to));

        if (match_from && match_to) {
            results[found++] = b->id;
        }
    }
    return found;
}

void bus_render_seat_map(const Bus *bus) {
    if (!bus) return;

    printf("\n  ========================================\n");
    printf("         SEAT MAP: %s\n", bus->name);
    printf("     Route: %s -> %s | Time: %s\n",
           bus->route_from, bus->route_to, bus->departure_time);
    printf("  ========================================\n");
    printf("             [ DRIVER CABIN ]\n\n");

    /* 2 + 2 layout */
    for (int i = 0; i < bus->total_seats; i += 4) {
        printf("    ");
        /* Left Window */
        if (i < bus->total_seats) {
            if (bus->seat_map[i]) printf("[XX] ");
            else printf("[%02d] ", i + 1);
        } else printf("     ");

        /* Left Aisle */
        if (i + 1 < bus->total_seats) {
            if (bus->seat_map[i + 1]) printf("[XX] ");
            else printf("[%02d] ", i + 2);
        } else printf("     ");

        printf("  ||  ");

        /* Right Aisle */
        if (i + 2 < bus->total_seats) {
            if (bus->seat_map[i + 2]) printf("[XX] ");
            else printf("[%02d] ", i + 3);
        } else printf("     ");

        /* Right Window */
        if (i + 3 < bus->total_seats) {
            if (bus->seat_map[i + 3]) printf("[XX]\n");
            else printf("[%02d]\n", i + 4);
        } else printf("\n");
    }

    printf("\n  Legend: [XX] = Booked | [01] = Available\n");
    printf("  Available: %d / %d seats\n",
           bus_available_seats_count(bus), bus->total_seats);
}

/* ------------------------------------------------------------------ */
/* Booking Operations                                                 */
/* ------------------------------------------------------------------ */

int booking_create(BusSystem *sys, int bus_id, const char *passenger_name,
                   const char *phone, int seat_number) {
    if (!sys || !passenger_name || passenger_name[0] == '\0') return -1;

    Bus *b = bus_find_by_id(sys, bus_id);
    if (!b) {
        fprintf(stderr, "Error: Bus #%d not found.\n", bus_id);
        return -1;
    }

    if (seat_number < 1 || seat_number > b->total_seats) {
        fprintf(stderr, "Error: Seat %d is out of range [1, %d].\n", seat_number, b->total_seats);
        return -1;
    }

    if (b->seat_map[seat_number - 1] != 0) {
        fprintf(stderr, "Error: Seat %d is already booked.\n", seat_number);
        return -1;
    }

    if (sys->booking_count >= sys->booking_capacity) {
        int new_cap = (sys->booking_capacity == 0) ? 8 : sys->booking_capacity * 2;
        Booking *grown = realloc(sys->bookings, (size_t)new_cap * sizeof(Booking));
        if (!grown) return -1;
        sys->bookings = grown;
        sys->booking_capacity = new_cap;
    }

    /* Reserve seat */
    b->seat_map[seat_number - 1] = 1;

    Booking *bk = &sys->bookings[sys->booking_count++];
    memset(bk, 0, sizeof(Booking));
    bk->id = sys->next_booking_id++;
    bk->bus_id = bus_id;
    strncpy(bk->passenger_name, passenger_name, sizeof(bk->passenger_name) - 1);
    if (phone) strncpy(bk->phone, phone, sizeof(bk->phone) - 1);
    bk->seat_number  = seat_number;
    bk->fare_paid    = b->fare;
    bk->booking_date = time(NULL);
    bk->is_cancelled = 0;

    return bk->id;
}

Booking *booking_find_by_id(BusSystem *sys, int id) {
    if (!sys || id <= 0) return NULL;
    for (int i = 0; i < sys->booking_count; i++) {
        if (sys->bookings[i].id == id) {
            return &sys->bookings[i];
        }
    }
    return NULL;
}

bool booking_cancel(BusSystem *sys, int booking_id) {
    Booking *bk = booking_find_by_id(sys, booking_id);
    if (!bk || bk->is_cancelled) return false;

    Bus *b = bus_find_by_id(sys, bk->bus_id);
    if (b && bk->seat_number >= 1 && bk->seat_number <= b->total_seats) {
        b->seat_map[bk->seat_number - 1] = 0; /* Release seat */
    }

    bk->is_cancelled = 1;
    return true;
}

void booking_print_ticket(const BusSystem *sys, int booking_id) {
    if (!sys) return;
    const Booking *bk = NULL;
    for (int i = 0; i < sys->booking_count; i++) {
        if (sys->bookings[i].id == booking_id) {
            bk = &sys->bookings[i];
            break;
        }
    }
    if (!bk) {
        printf("  Booking #%d not found.\n", booking_id);
        return;
    }

    const Bus *b = NULL;
    for (int i = 0; i < sys->bus_count; i++) {
        if (sys->buses[i].id == bk->bus_id) {
            b = &sys->buses[i];
            break;
        }
    }

    char dstr[32];
    struct tm *tm_info = localtime(&bk->booking_date);
    strftime(dstr, sizeof(dstr), "%Y-%m-%d %H:%M", tm_info);

    printf("\n");
    printf("  +-------------------------------------------------------------+\n");
    printf("  |                     BOARDING PASS / TICKET                  |\n");
    printf("  +-------------------------------------------------------------+\n");
    printf("  | Ticket ID: #%-10d Status: %-25s |\n",
           bk->id, bk->is_cancelled ? "CANCELLED ❌" : "CONFIRMED ✅");
    printf("  | Passenger: %-25s Phone: %-15s |\n", bk->passenger_name, bk->phone);
    if (b) {
        printf("  | Operator:  %-48s |\n", b->name);
        printf("  | Route:     %-15s -> %-28s |\n", b->route_from, b->route_to);
        printf("  | Departure: %-15s Seat Number: [%02d]               |\n",
               b->departure_time, bk->seat_number);
    }
    printf("  | Fare Paid: $%-14.2f Booked On: %-19s |\n", bk->fare_paid, dstr);
    printf("  +-------------------------------------------------------------+\n");
}

void booking_list_by_bus(const BusSystem *sys, int bus_id) {
    if (!sys) return;
    printf("\n  Bookings for Bus #%d:\n", bus_id);
    int count = 0;
    for (int i = 0; i < sys->booking_count; i++) {
        const Booking *bk = &sys->bookings[i];
        if (bk->bus_id == bus_id && !bk->is_cancelled) {
            printf("    Ticket #%d: %s | Seat: %02d | Phone: %s\n",
                   bk->id, bk->passenger_name, bk->seat_number, bk->phone);
            count++;
        }
    }
    if (count == 0) printf("    No active bookings.\n");
}
