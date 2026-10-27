#include "zoo.h"
#include "file_ops.h"
#include "input.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define FILE_ANIMALS "animals.dat"
#define FILE_ENCLOS  "enclosures.dat"
#define FILE_STAFF   "staff.dat"
#define FILE_TICKETS "tickets.dat"

void zoo_init(Zoo *zoo) {
    if (!zoo) return;
    memset(zoo, 0, sizeof(Zoo));
    zoo->next_animal_id    = 1;
    zoo->next_enclosure_id = 1;
    zoo->next_staff_id     = 1;
    zoo->next_ticket_id    = 1;

    const char *env = getenv("DATA_DIR");
    if (env && env[0] != '\0') {
        snprintf(zoo->data_dir, sizeof(zoo->data_dir), "%s", env);
    } else {
        snprintf(zoo->data_dir, sizeof(zoo->data_dir), "./data");
    }
}

void zoo_free(Zoo *zoo) {
    if (!zoo) return;
    free(zoo->animals);
    free(zoo->enclosures);
    free(zoo->staff);
    free(zoo->tickets);
    memset(zoo, 0, sizeof(Zoo));
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

bool zoo_load(Zoo *zoo) {
    if (!zoo) return false;
    if (!util_ensure_dir(zoo->data_dir)) return false;

    void *data;
    uint32_t count, next_id;

    /* Enclosures */
    load_records(zoo->data_dir, FILE_ENCLOS, ZOO_MAGIC_ENCLOSURE, &data, &count, &next_id, sizeof(Enclosure));
    zoo->enclosures        = (Enclosure *)data;
    zoo->enclosure_count   = (int)count;
    zoo->enclosure_capacity= (int)count;
    zoo->next_enclosure_id = (int)next_id;

    /* Animals */
    load_records(zoo->data_dir, FILE_ANIMALS, ZOO_MAGIC_ANIMAL, &data, &count, &next_id, sizeof(Animal));
    zoo->animals           = (Animal *)data;
    zoo->animal_count      = (int)count;
    zoo->animal_capacity   = (int)count;
    zoo->next_animal_id    = (int)next_id;

    /* Staff */
    load_records(zoo->data_dir, FILE_STAFF, ZOO_MAGIC_STAFF, &data, &count, &next_id, sizeof(Staff));
    zoo->staff             = (Staff *)data;
    zoo->staff_count       = (int)count;
    zoo->staff_capacity    = (int)count;
    zoo->next_staff_id     = (int)next_id;

    /* Tickets */
    load_records(zoo->data_dir, FILE_TICKETS, ZOO_MAGIC_TICKET, &data, &count, &next_id, sizeof(TicketSale));
    zoo->tickets           = (TicketSale *)data;
    zoo->ticket_count      = (int)count;
    zoo->ticket_capacity   = (int)count;
    zoo->next_ticket_id    = (int)next_id;

    return true;
}

bool zoo_save(Zoo *zoo) {
    if (!zoo) return false;
    char path[512];

    snprintf(path, sizeof(path), "%s/%s", zoo->data_dir, FILE_ENCLOS);
    file_atomic_save(path, ZOO_MAGIC_ENCLOSURE, zoo->enclosures, sizeof(Enclosure),
                     (uint32_t)zoo->enclosure_count, (uint32_t)zoo->next_enclosure_id);

    snprintf(path, sizeof(path), "%s/%s", zoo->data_dir, FILE_ANIMALS);
    file_atomic_save(path, ZOO_MAGIC_ANIMAL, zoo->animals, sizeof(Animal),
                     (uint32_t)zoo->animal_count, (uint32_t)zoo->next_animal_id);

    snprintf(path, sizeof(path), "%s/%s", zoo->data_dir, FILE_STAFF);
    file_atomic_save(path, ZOO_MAGIC_STAFF, zoo->staff, sizeof(Staff),
                     (uint32_t)zoo->staff_count, (uint32_t)zoo->next_staff_id);

    snprintf(path, sizeof(path), "%s/%s", zoo->data_dir, FILE_TICKETS);
    file_atomic_save(path, ZOO_MAGIC_TICKET, zoo->tickets, sizeof(TicketSale),
                     (uint32_t)zoo->ticket_count, (uint32_t)zoo->next_ticket_id);

    return true;
}

/* ------------------------------------------------------------------ */
/* Enclosure Operations                                               */
/* ------------------------------------------------------------------ */

int enclosure_add(Zoo *zoo, const char *name, const char *type, int capacity) {
    if (!zoo || !name || !type) return -1;

    if (zoo->enclosure_count >= zoo->enclosure_capacity) {
        int new_cap = (zoo->enclosure_capacity == 0) ? 8 : zoo->enclosure_capacity * 2;
        Enclosure *grown = realloc(zoo->enclosures, (size_t)new_cap * sizeof(Enclosure));
        if (!grown) return -1;
        zoo->enclosures = grown;
        zoo->enclosure_capacity = new_cap;
    }

    Enclosure *enc = &zoo->enclosures[zoo->enclosure_count++];
    memset(enc, 0, sizeof(Enclosure));
    enc->id = zoo->next_enclosure_id++;
    strncpy(enc->name, name, sizeof(enc->name) - 1);
    strncpy(enc->type, type, sizeof(enc->type) - 1);
    enc->capacity = capacity > 0 ? capacity : 10;
    enc->current_count = 0;
    enc->is_deleted = 0;

    return enc->id;
}

Enclosure *enclosure_find_by_id(Zoo *zoo, int id) {
    if (!zoo || id <= 0) return NULL;
    for (int i = 0; i < zoo->enclosure_count; i++) {
        if (zoo->enclosures[i].id == id && !zoo->enclosures[i].is_deleted) {
            return &zoo->enclosures[i];
        }
    }
    return NULL;
}

bool enclosure_delete(Zoo *zoo, int id) {
    Enclosure *e = enclosure_find_by_id(zoo, id);
    if (!e) return false;
    e->is_deleted = 1;
    return true;
}

void enclosure_list_all(const Zoo *zoo) {
    if (!zoo) return;
    printf("\n");
    util_print_separator(70);
    printf("  %-4s | %-22s | %-16s | %s\n", "ID", "Enclosure Name", "Type / Habitat", "Occupancy");
    util_print_separator(70);

    int count = 0;
    for (int i = 0; i < zoo->enclosure_count; i++) {
        const Enclosure *e = &zoo->enclosures[i];
        if (e->is_deleted) continue;
        printf("  %-4d | %-22.22s | %-16.16s | %d / %d animal(s)\n",
               e->id, e->name, e->type, e->current_count, e->capacity);
        count++;
    }
    util_print_separator(70);
    printf("  Total: %d enclosure(s)\n", count);
}

/* ------------------------------------------------------------------ */
/* Animal Operations                                                  */
/* ------------------------------------------------------------------ */

int animal_add(Zoo *zoo, const char *name, const char *species, int age,
               int enclosure_id, const char *feeding) {
    if (!zoo || !name || !species) return -1;

    Enclosure *enc = enclosure_find_by_id(zoo, enclosure_id);
    if (enc && enc->current_count >= enc->capacity) {
        fprintf(stderr, "Error: Enclosure '%s' is at maximum capacity (%d)!\n",
                enc->name, enc->capacity);
        return -1;
    }

    if (zoo->animal_count >= zoo->animal_capacity) {
        int new_cap = (zoo->animal_capacity == 0) ? 8 : zoo->animal_capacity * 2;
        Animal *grown = realloc(zoo->animals, (size_t)new_cap * sizeof(Animal));
        if (!grown) return -1;
        zoo->animals = grown;
        zoo->animal_capacity = new_cap;
    }

    Animal *a = &zoo->animals[zoo->animal_count++];
    memset(a, 0, sizeof(Animal));
    a->id = zoo->next_animal_id++;
    strncpy(a->name, name, sizeof(a->name) - 1);
    strncpy(a->species, species, sizeof(a->species) - 1);
    a->age = age >= 0 ? age : 1;
    a->enclosure_id = enclosure_id;
    if (feeding) strncpy(a->feeding_schedule, feeding, sizeof(a->feeding_schedule) - 1);
    else strncpy(a->feeding_schedule, "10:00 Feed", sizeof(a->feeding_schedule) - 1);
    a->is_deleted = 0;

    if (enc) enc->current_count++;
    return a->id;
}

Animal *animal_find_by_id(Zoo *zoo, int id) {
    if (!zoo || id <= 0) return NULL;
    for (int i = 0; i < zoo->animal_count; i++) {
        if (zoo->animals[i].id == id && !zoo->animals[i].is_deleted) {
            return &zoo->animals[i];
        }
    }
    return NULL;
}

bool animal_transfer(Zoo *zoo, int animal_id, int new_enclosure_id) {
    Animal *a = animal_find_by_id(zoo, animal_id);
    if (!a) return false;

    Enclosure *new_enc = enclosure_find_by_id(zoo, new_enclosure_id);
    if (!new_enc || new_enc->current_count >= new_enc->capacity) return false;

    Enclosure *old_enc = enclosure_find_by_id(zoo, a->enclosure_id);
    if (old_enc && old_enc->current_count > 0) old_enc->current_count--;

    new_enc->current_count++;
    a->enclosure_id = new_enclosure_id;
    return true;
}

bool animal_delete(Zoo *zoo, int id) {
    Animal *a = animal_find_by_id(zoo, id);
    if (!a) return false;

    Enclosure *enc = enclosure_find_by_id(zoo, a->enclosure_id);
    if (enc && enc->current_count > 0) enc->current_count--;

    a->is_deleted = 1;
    return true;
}

void animal_list_all(const Zoo *zoo) {
    if (!zoo) return;
    printf("\n");
    util_print_separator(78);
    printf("  %-4s | %-18s | %-16s | %-3s | %-16s | %s\n",
           "ID", "Animal Name", "Species", "Age", "Enclosure", "Feeding Schedule");
    util_print_separator(78);

    int count = 0;
    for (int i = 0; i < zoo->animal_count; i++) {
        const Animal *a = &zoo->animals[i];
        if (a->is_deleted) continue;

        const char *enc_name = "Unassigned";
        for (int e = 0; e < zoo->enclosure_count; e++) {
            if (zoo->enclosures[e].id == a->enclosure_id) {
                enc_name = zoo->enclosures[e].name;
                break;
            }
        }

        printf("  %-4d | %-18.18s | %-16.16s | %-3d | %-16.16s | %s\n",
               a->id, a->name, a->species, a->age, enc_name, a->feeding_schedule);
        count++;
    }
    util_print_separator(78);
    printf("  Total: %d animal(s)\n", count);
}

int animal_search(const Zoo *zoo, const char *query, int *results, int max_results) {
    if (!zoo || !query || !results || max_results <= 0) return 0;
    int found = 0;
    for (int i = 0; i < zoo->animal_count && found < max_results; i++) {
        const Animal *a = &zoo->animals[i];
        if (a->is_deleted) continue;
        if (str_contains_ci(a->name, query) || str_contains_ci(a->species, query)) {
            results[found++] = a->id;
        }
    }
    return found;
}

/* ------------------------------------------------------------------ */
/* Staff Operations                                                   */
/* ------------------------------------------------------------------ */

int staff_add(Zoo *zoo, const char *name, const char *role, const char *phone, int enclosure_id) {
    if (!zoo || !name || !role) return -1;

    if (zoo->staff_count >= zoo->staff_capacity) {
        int new_cap = (zoo->staff_capacity == 0) ? 8 : zoo->staff_capacity * 2;
        Staff *grown = realloc(zoo->staff, (size_t)new_cap * sizeof(Staff));
        if (!grown) return -1;
        zoo->staff = grown;
        zoo->staff_capacity = new_cap;
    }

    Staff *s = &zoo->staff[zoo->staff_count++];
    memset(s, 0, sizeof(Staff));
    s->id = zoo->next_staff_id++;
    strncpy(s->name, name, sizeof(s->name) - 1);
    strncpy(s->role, role, sizeof(s->role) - 1);
    if (phone) strncpy(s->phone, phone, sizeof(s->phone) - 1);
    s->assigned_enclosure_id = enclosure_id;
    s->is_deleted = 0;

    return s->id;
}

Staff *staff_find_by_id(Zoo *zoo, int id) {
    if (!zoo || id <= 0) return NULL;
    for (int i = 0; i < zoo->staff_count; i++) {
        if (zoo->staff[i].id == id && !zoo->staff[i].is_deleted) {
            return &zoo->staff[i];
        }
    }
    return NULL;
}

bool staff_delete(Zoo *zoo, int id) {
    Staff *s = staff_find_by_id(zoo, id);
    if (!s) return false;
    s->is_deleted = 1;
    return true;
}

void staff_list_all(const Zoo *zoo) {
    if (!zoo) return;
    printf("\n");
    util_print_separator(70);
    printf("  %-4s | %-20s | %-12s | %-14s | %s\n",
           "ID", "Staff Member", "Role", "Phone", "Assigned Habitat");
    util_print_separator(70);

    int count = 0;
    for (int i = 0; i < zoo->staff_count; i++) {
        const Staff *s = &zoo->staff[i];
        if (s->is_deleted) continue;

        const char *enc_name = "General";
        for (int e = 0; e < zoo->enclosure_count; e++) {
            if (zoo->enclosures[e].id == s->assigned_enclosure_id) {
                enc_name = zoo->enclosures[e].name;
                break;
            }
        }

        printf("  %-4d | %-20.20s | %-12.12s | %-14.14s | %s\n",
               s->id, s->name, s->role, s->phone, enc_name);
        count++;
    }
    util_print_separator(70);
    printf("  Total: %d staff member(s)\n", count);
}

/* ------------------------------------------------------------------ */
/* Ticketing Operations                                               */
/* ------------------------------------------------------------------ */

int ticket_sell(Zoo *zoo, int count, int ticket_type) {
    if (!zoo || count <= 0) return -1;

    double price_per_ticket = (ticket_type == 2) ? 12.0 :  /* Child */
                              (ticket_type == 3) ? 15.0 :  /* Senior */
                                                   20.0;   /* Adult */

    if (zoo->ticket_count >= zoo->ticket_capacity) {
        int new_cap = (zoo->ticket_capacity == 0) ? 8 : zoo->ticket_capacity * 2;
        TicketSale *grown = realloc(zoo->tickets, (size_t)new_cap * sizeof(TicketSale));
        if (!grown) return -1;
        zoo->tickets = grown;
        zoo->ticket_capacity = new_cap;
    }

    TicketSale *t = &zoo->tickets[zoo->ticket_count++];
    t->id           = zoo->next_ticket_id++;
    t->date         = time(NULL);
    t->tickets_sold = count;
    t->ticket_type  = ticket_type;
    t->revenue      = (double)count * price_per_ticket;

    return t->id;
}

double ticket_total_revenue(const Zoo *zoo) {
    if (!zoo) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < zoo->ticket_count; i++) {
        sum += zoo->tickets[i].revenue;
    }
    return sum;
}

void ticket_sales_report(const Zoo *zoo) {
    if (!zoo) return;
    printf("\n");
    util_print_separator(65);
    printf("  %-4s | %-12s | %-8s | %-12s | %s\n",
           "ID", "Date", "Quantity", "Tier", "Total Revenue");
    util_print_separator(65);

    int total_tickets = 0;
    double total_revenue = 0.0;

    for (int i = 0; i < zoo->ticket_count; i++) {
        const TicketSale *t = &zoo->tickets[i];
        char dstr[32];
        struct tm *tm_info = localtime(&t->date);
        strftime(dstr, sizeof(dstr), "%Y-%m-%d", tm_info);

        const char *tier = (t->ticket_type == 2) ? "Child ($12)" :
                           (t->ticket_type == 3) ? "Senior ($15)" : "Adult ($20)";

        printf("  %-4d | %-12s | %-8d | %-12s | $%.2f\n",
               t->id, dstr, t->tickets_sold, tier, t->revenue);

        total_tickets += t->tickets_sold;
        total_revenue += t->revenue;
    }
    util_print_separator(65);
    printf("  Total Visitors: %d | Total Revenue Collected: $%.2f\n",
           total_tickets, total_revenue);
}
