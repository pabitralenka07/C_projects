#ifndef ZOO_H
#define ZOO_H

#include <stdbool.h>
#include <time.h>
#include <stdint.h>
#include <stddef.h>

#define ANIMAL_NAME_LEN     64
#define ANIMAL_SPECIES_LEN  64
#define ANIMAL_FEED_LEN     128
#define ENCLOSURE_NAME_LEN  32
#define ENCLOSURE_TYPE_LEN  32
#define STAFF_NAME_LEN      64
#define STAFF_ROLE_LEN      32
#define STAFF_PHONE_LEN     16

#define ZOO_MAGIC_ANIMAL    "ZOOa"
#define ZOO_MAGIC_ENCLOSURE "ZOOe"
#define ZOO_MAGIC_STAFF     "ZOOs"
#define ZOO_MAGIC_TICKET    "ZOOt"

typedef struct {
    int  id;
    char name[ANIMAL_NAME_LEN];
    char species[ANIMAL_SPECIES_LEN];
    int  age;
    int  enclosure_id;
    char feeding_schedule[ANIMAL_FEED_LEN];
    int  is_deleted;
} Animal;

typedef struct {
    int  id;
    char name[ENCLOSURE_NAME_LEN];
    char type[ENCLOSURE_TYPE_LEN];  /* "Aviary", "Aquarium", "Open Plains", "Safari" */
    int  capacity;
    int  current_count;
    int  is_deleted;
} Enclosure;

typedef struct {
    int  id;
    char name[STAFF_NAME_LEN];
    char role[STAFF_ROLE_LEN];      /* "Keeper", "Vet", "Guide", "Curator" */
    char phone[STAFF_PHONE_LEN];
    int  assigned_enclosure_id;
    int  is_deleted;
} Staff;

typedef struct {
    int    id;
    time_t date;
    int    tickets_sold;
    int    ticket_type;            /* 1 = Adult ($20), 2 = Child ($12), 3 = Senior ($15) */
    double revenue;
} TicketSale;

typedef struct {
    Animal     *animals;
    int         animal_count;
    int         animal_capacity;
    int         next_animal_id;

    Enclosure  *enclosures;
    int         enclosure_count;
    int         enclosure_capacity;
    int         next_enclosure_id;

    Staff      *staff;
    int         staff_count;
    int         staff_capacity;
    int         next_staff_id;

    TicketSale *tickets;
    int         ticket_count;
    int         ticket_capacity;
    int         next_ticket_id;

    char        data_dir[256];
} Zoo;

/* Lifecycle */
void zoo_init(Zoo *zoo);
void zoo_free(Zoo *zoo);
bool zoo_load(Zoo *zoo);
bool zoo_save(Zoo *zoo);

/* Animal operations */
int     animal_add(Zoo *zoo, const char *name, const char *species, int age,
                   int enclosure_id, const char *feeding);
Animal *animal_find_by_id(Zoo *zoo, int id);
bool    animal_transfer(Zoo *zoo, int animal_id, int new_enclosure_id);
bool    animal_delete(Zoo *zoo, int id);
void    animal_list_all(const Zoo *zoo);
int     animal_search(const Zoo *zoo, const char *query, int *results, int max_results);

/* Enclosure operations */
int        enclosure_add(Zoo *zoo, const char *name, const char *type, int capacity);
Enclosure *enclosure_find_by_id(Zoo *zoo, int id);
bool       enclosure_delete(Zoo *zoo, int id);
void       enclosure_list_all(const Zoo *zoo);

/* Staff operations */
int    staff_add(Zoo *zoo, const char *name, const char *role, const char *phone, int enclosure_id);
Staff *staff_find_by_id(Zoo *zoo, int id);
bool   staff_delete(Zoo *zoo, int id);
void   staff_list_all(const Zoo *zoo);

/* Ticketing operations */
int    ticket_sell(Zoo *zoo, int count, int ticket_type);
void   ticket_sales_report(const Zoo *zoo);
double ticket_total_revenue(const Zoo *zoo);

#endif /* ZOO_H */
