#include "airport.h"
#include "file_ops.h"
#include "input.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define FILE_FLIGHTS "flights.dat"
#define FILE_PASSENG "passengers.dat"

void airport_init(Airport *air) {
    if (!air) return;
    memset(air, 0, sizeof(Airport));
    air->next_flight_id    = 1;
    air->next_passenger_id = 1;

    const char *env = getenv("DATA_DIR");
    if (env && env[0] != '\0') {
        snprintf(air->data_dir, sizeof(air->data_dir), "%s", env);
    } else {
        snprintf(air->data_dir, sizeof(air->data_dir), "./data");
    }
}

void airport_free(Airport *air) {
    if (!air) return;
    free(air->flights);
    free(air->passengers);
    memset(air, 0, sizeof(Airport));
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

bool airport_load(Airport *air) {
    if (!air) return false;
    if (!util_ensure_dir(air->data_dir)) return false;

    void *data;
    uint32_t count, next_id;

    /* Flights */
    load_records(air->data_dir, FILE_FLIGHTS, AIR_MAGIC_FLIGHT, &data, &count, &next_id, sizeof(Flight));
    air->flights        = (Flight *)data;
    air->flight_count   = (int)count;
    air->flight_capacity= (int)count;
    air->next_flight_id = (int)next_id;

    /* Passengers */
    load_records(air->data_dir, FILE_PASSENG, AIR_MAGIC_PASS, &data, &count, &next_id, sizeof(Passenger));
    air->passengers        = (Passenger *)data;
    air->passenger_count   = (int)count;
    air->passenger_capacity= (int)count;
    air->next_passenger_id = (int)next_id;

    return true;
}

bool airport_save(Airport *air) {
    if (!air) return false;
    char path[512];

    snprintf(path, sizeof(path), "%s/%s", air->data_dir, FILE_FLIGHTS);
    file_atomic_save(path, AIR_MAGIC_FLIGHT, air->flights, sizeof(Flight),
                     (uint32_t)air->flight_count, (uint32_t)air->next_flight_id);

    snprintf(path, sizeof(path), "%s/%s", air->data_dir, FILE_PASSENG);
    file_atomic_save(path, AIR_MAGIC_PASS, air->passengers, sizeof(Passenger),
                     (uint32_t)air->passenger_count, (uint32_t)air->next_passenger_id);

    return true;
}

/* ------------------------------------------------------------------ */
/* Flight Operations                                                  */
/* ------------------------------------------------------------------ */

int flight_add(Airport *air, const char *num, const char *airline,
               const char *origin, const char *dest, const char *dept,
               const char *arr, int gate, int capacity) {
    if (!air || !num || !origin || !dest) return -1;

    if (air->flight_count >= air->flight_capacity) {
        int new_cap = (air->flight_capacity == 0) ? 8 : air->flight_capacity * 2;
        Flight *grown = realloc(air->flights, (size_t)new_cap * sizeof(Flight));
        if (!grown) return -1;
        air->flights = grown;
        air->flight_capacity = new_cap;
    }

    Flight *f = &air->flights[air->flight_count++];
    memset(f, 0, sizeof(Flight));
    f->id = air->next_flight_id++;
    strncpy(f->flight_number, num, sizeof(f->flight_number) - 1);
    if (airline) strncpy(f->airline, airline, sizeof(f->airline) - 1);
    strncpy(f->origin, origin, sizeof(f->origin) - 1);
    strncpy(f->destination, dest, sizeof(f->destination) - 1);
    if (dept) strncpy(f->departure, dept, sizeof(f->departure) - 1);
    if (arr) strncpy(f->arrival, arr, sizeof(f->arrival) - 1);
    f->gate = gate > 0 ? gate : 1;
    strncpy(f->status, "On Time", sizeof(f->status) - 1);
    f->capacity = capacity > 0 ? capacity : 150;
    f->checked_in = 0;
    f->is_deleted = 0;

    return f->id;
}

Flight *flight_find_by_id(Airport *air, int id) {
    if (!air || id <= 0) return NULL;
    for (int i = 0; i < air->flight_count; i++) {
        if (air->flights[i].id == id && !air->flights[i].is_deleted) {
            return &air->flights[i];
        }
    }
    return NULL;
}

Flight *flight_find_by_number(Airport *air, const char *num) {
    if (!air || !num) return NULL;
    for (int i = 0; i < air->flight_count; i++) {
        if (!air->flights[i].is_deleted &&
            strcasecmp(air->flights[i].flight_number, num) == 0) {
            return &air->flights[i];
        }
    }
    return NULL;
}

bool flight_update_status(Airport *air, int flight_id, const char *new_status) {
    Flight *f = flight_find_by_id(air, flight_id);
    if (!f || !new_status) return false;
    strncpy(f->status, new_status, sizeof(f->status) - 1);
    return true;
}

bool flight_update_gate(Airport *air, int flight_id, int new_gate) {
    Flight *f = flight_find_by_id(air, flight_id);
    if (!f || new_gate <= 0) return false;
    f->gate = new_gate;
    return true;
}

bool flight_delete(Airport *air, int id) {
    Flight *f = flight_find_by_id(air, id);
    if (!f) return false;
    f->is_deleted = 1;
    return true;
}

void flight_list_board(const Airport *air) {
    if (!air) return;
    printf("\n");
    util_print_separator(82);
    printf("  %-4s | %-8s | %-16s | %-12s | %-12s | %-5s | %-4s | %s\n",
           "ID", "Flight", "Airline", "Origin", "Destination", "Time", "Gate", "Status");
    util_print_separator(82);

    int count = 0;
    for (int i = 0; i < air->flight_count; i++) {
        const Flight *f = &air->flights[i];
        if (f->is_deleted) continue;

        printf("  %-4d | %-8s | %-16.16s | %-12.12s | %-12.12s | %-5s | %-4d | %s\n",
               f->id, f->flight_number, f->airline,
               f->origin, f->destination, f->departure, f->gate, f->status);
        count++;
    }
    util_print_separator(82);
    printf("  Total: %d flight(s) on board\n", count);
}

int flight_search(const Airport *air, const char *query, int *results, int max_results) {
    if (!air || !query || !results || max_results <= 0) return 0;
    int found = 0;
    for (int i = 0; i < air->flight_count && found < max_results; i++) {
        const Flight *f = &air->flights[i];
        if (f->is_deleted) continue;
        if (str_contains_ci(f->flight_number, query) ||
            str_contains_ci(f->airline, query) ||
            str_contains_ci(f->origin, query) ||
            str_contains_ci(f->destination, query)) {
            results[found++] = f->id;
        }
    }
    return found;
}

/* ------------------------------------------------------------------ */
/* Passenger & Check-In                                               */
/* ------------------------------------------------------------------ */

int passenger_book(Airport *air, const char *name, const char *passport, int flight_id) {
    if (!air || !name || name[0] == '\0') return -1;

    Flight *f = flight_find_by_id(air, flight_id);
    if (!f) {
        fprintf(stderr, "Error: Flight #%d not found.\n", flight_id);
        return -1;
    }

    if (air->passenger_count >= air->passenger_capacity) {
        int new_cap = (air->passenger_capacity == 0) ? 8 : air->passenger_capacity * 2;
        Passenger *grown = realloc(air->passengers, (size_t)new_cap * sizeof(Passenger));
        if (!grown) return -1;
        air->passengers = grown;
        air->passenger_capacity = new_cap;
    }

    /* Assign next seat number for this flight */
    int booked_on_flight = 0;
    for (int i = 0; i < air->passenger_count; i++) {
        if (air->passengers[i].flight_id == flight_id && !air->passengers[i].is_deleted) {
            booked_on_flight++;
        }
    }

    if (booked_on_flight >= f->capacity) {
        fprintf(stderr, "Error: Flight %s is fully booked!\n", f->flight_number);
        return -1;
    }

    Passenger *p = &air->passengers[air->passenger_count++];
    memset(p, 0, sizeof(Passenger));
    p->id = air->next_passenger_id++;
    strncpy(p->name, name, sizeof(p->name) - 1);
    if (passport) strncpy(p->passport, passport, sizeof(p->passport) - 1);
    p->flight_id = flight_id;
    p->seat_number = booked_on_flight + 1;
    p->checked_in = 0;
    p->is_deleted = 0;

    return p->id;
}

Passenger *passenger_find_by_id(Airport *air, int id) {
    if (!air || id <= 0) return NULL;
    for (int i = 0; i < air->passenger_count; i++) {
        if (air->passengers[i].id == id && !air->passengers[i].is_deleted) {
            return &air->passengers[i];
        }
    }
    return NULL;
}

bool passenger_checkin(Airport *air, int passenger_id) {
    Passenger *p = passenger_find_by_id(air, passenger_id);
    if (!p || p->checked_in) return false;

    Flight *f = flight_find_by_id(air, p->flight_id);
    if (f) f->checked_in++;

    p->checked_in = 1;
    return true;
}

void passenger_print_boarding_pass(const Airport *air, int passenger_id) {
    const Passenger *p = NULL;
    for (int i = 0; i < air->passenger_count; i++) {
        if (air->passengers[i].id == passenger_id) {
            p = &air->passengers[i];
            break;
        }
    }
    if (!p) {
        printf("  Passenger #%d not found.\n", passenger_id);
        return;
    }

    const Flight *f = NULL;
    for (int i = 0; i < air->flight_count; i++) {
        if (air->flights[i].id == p->flight_id) {
            f = &air->flights[i];
            break;
        }
    }

    printf("\n");
    printf("  +-------------------------------------------------------------+\n");
    printf("  |                     AIRPORT BOARDING PASS                   |\n");
    printf("  +-------------------------------------------------------------+\n");
    printf("  | Pass ID: #%-10d Status: %-25s |\n",
           p->id, p->checked_in ? "CHECKED IN ✅" : "PENDING CHECK-IN ⏳");
    printf("  | Passenger: %-25s Passport: %-15s |\n", p->name, p->passport);
    if (f) {
        printf("  | Airline:   %-25s Flight:   %-15s |\n", f->airline, f->flight_number);
        printf("  | From:      %-25s To:       %-15s |\n", f->origin, f->destination);
        printf("  | Departure: %-25s Gate:     %-3d Seat: [%02d]    |\n",
               f->departure, f->gate, p->seat_number);
    }
    printf("  +-------------------------------------------------------------+\n");
}

void passenger_manifest_by_flight(const Airport *air, int flight_id) {
    if (!air) return;
    Flight *f = flight_find_by_id((Airport *)air, flight_id);
    if (!f) {
        printf("  Flight #%d not found.\n", flight_id);
        return;
    }

    printf("\n  Manifest for Flight %s (%s -> %s):\n",
           f->flight_number, f->origin, f->destination);
    printf("  Checked-in passengers: %d / %d capacity\n", f->checked_in, f->capacity);
    util_print_separator(65);

    int count = 0;
    for (int i = 0; i < air->passenger_count; i++) {
        const Passenger *p = &air->passengers[i];
        if (p->flight_id == flight_id && !p->is_deleted) {
            printf("    Seat [%02d] #%-4d %-22.22s (Passport: %-12s) [%s]\n",
                   p->seat_number, p->id, p->name, p->passport,
                   p->checked_in ? "BOARDED" : "UNCONFIRMED");
            count++;
        }
    }
    if (count == 0) printf("    No booked passengers.\n");
    util_print_separator(65);
}
