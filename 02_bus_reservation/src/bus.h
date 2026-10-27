#ifndef BUS_H
#define BUS_H

#include <stdbool.h>
#include <time.h>
#include <stdint.h>
#include <stddef.h>

#define BUS_NAME_LEN        64
#define BUS_CITY_LEN        32
#define BUS_TIME_LEN        16
#define BUS_MAX_SEATS       60

#define PASS_NAME_LEN       64
#define PASS_PHONE_LEN      16

#define BUS_MAGIC_BUS       "BUSb"
#define BUS_MAGIC_BOOK      "BUSk"

typedef struct {
    int    id;
    char   name[BUS_NAME_LEN];
    char   route_from[BUS_CITY_LEN];
    char   route_to[BUS_CITY_LEN];
    char   departure_time[BUS_TIME_LEN];
    double fare;
    int    total_seats;               /* up to BUS_MAX_SEATS */
    int    seat_map[BUS_MAX_SEATS];   /* 0 = available, 1 = booked */
    int    is_deleted;
} Bus;

typedef struct {
    int    id;
    int    bus_id;
    char   passenger_name[PASS_NAME_LEN];
    char   phone[PASS_PHONE_LEN];
    int    seat_number;               /* 1-indexed */
    double fare_paid;
    time_t booking_date;
    int    is_cancelled;
} Booking;

typedef struct {
    Bus     *buses;
    int      bus_count;
    int      bus_capacity;
    int      next_bus_id;

    Booking *bookings;
    int      booking_count;
    int      booking_capacity;
    int      next_booking_id;

    char     data_dir[256];
} BusSystem;

/* Lifecycle */
void bus_system_init(BusSystem *sys);
void bus_system_free(BusSystem *sys);
bool bus_system_load(BusSystem *sys);
bool bus_system_save(BusSystem *sys);

/* Bus operations */
int  bus_add(BusSystem *sys, const char *name, const char *from, const char *to,
             const char *time_str, double fare, int total_seats);
Bus *bus_find_by_id(BusSystem *sys, int id);
bool bus_delete(BusSystem *sys, int id);
void bus_list_all(const BusSystem *sys);
int  bus_search_route(const BusSystem *sys, const char *from, const char *to,
                      int *results, int max_results);
void bus_render_seat_map(const Bus *bus);
int  bus_available_seats_count(const Bus *bus);

/* Booking operations */
int      booking_create(BusSystem *sys, int bus_id, const char *passenger_name,
                        const char *phone, int seat_number);
bool     booking_cancel(BusSystem *sys, int booking_id);
Booking *booking_find_by_id(BusSystem *sys, int id);
void     booking_print_ticket(const BusSystem *sys, int booking_id);
void     booking_list_by_bus(const BusSystem *sys, int bus_id);

#endif /* BUS_H */
