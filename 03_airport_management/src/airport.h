#ifndef AIRPORT_H
#define AIRPORT_H

#include <stdbool.h>
#include <time.h>
#include <stdint.h>
#include <stddef.h>

#define FLIGHT_NUM_LEN    16
#define AIRLINE_NAME_LEN  32
#define CITY_NAME_LEN     32
#define TIME_STR_LEN      16
#define STATUS_STR_LEN    16

#define PASS_NAME_LEN     64
#define PASSPORT_LEN      20

#define AIR_MAGIC_FLIGHT  "AIRf"
#define AIR_MAGIC_PASS    "AIRp"

typedef struct {
    int  id;
    char flight_number[FLIGHT_NUM_LEN];
    char airline[AIRLINE_NAME_LEN];
    char origin[CITY_NAME_LEN];
    char destination[CITY_NAME_LEN];
    char departure[TIME_STR_LEN];
    char arrival[TIME_STR_LEN];
    int  gate;
    char status[STATUS_STR_LEN];    /* "On Time", "Delayed", "Boarding", "Departed", "Cancelled" */
    int  capacity;
    int  checked_in;
    int  is_deleted;
} Flight;

typedef struct {
    int  id;
    char name[PASS_NAME_LEN];
    char passport[PASSPORT_LEN];
    int  flight_id;
    int  seat_number;
    int  checked_in;                /* 0 = booked/pending, 1 = checked in */
    int  is_deleted;
} Passenger;

typedef struct {
    Flight    *flights;
    int        flight_count;
    int        flight_capacity;
    int        next_flight_id;

    Passenger *passengers;
    int        passenger_count;
    int        passenger_capacity;
    int        next_passenger_id;

    char       data_dir[256];
} Airport;

/* Lifecycle */
void airport_init(Airport *air);
void airport_free(Airport *air);
bool airport_load(Airport *air);
bool airport_save(Airport *air);

/* Flight operations */
int     flight_add(Airport *air, const char *num, const char *airline,
                   const char *origin, const char *dest, const char *dept,
                   const char *arr, int gate, int capacity);
Flight *flight_find_by_id(Airport *air, int id);
Flight *flight_find_by_number(Airport *air, const char *num);
bool    flight_update_status(Airport *air, int flight_id, const char *new_status);
bool    flight_update_gate(Airport *air, int flight_id, int new_gate);
bool    flight_delete(Airport *air, int id);
void    flight_list_board(const Airport *air);
int     flight_search(const Airport *air, const char *query, int *results, int max_results);

/* Passenger & Check-In operations */
int        passenger_book(Airport *air, const char *name, const char *passport, int flight_id);
Passenger *passenger_find_by_id(Airport *air, int id);
bool       passenger_checkin(Airport *air, int passenger_id);
void       passenger_print_boarding_pass(const Airport *air, int passenger_id);
void       passenger_manifest_by_flight(const Airport *air, int flight_id);

#endif /* AIRPORT_H */
