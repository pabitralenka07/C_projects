#include "hospital.h"
#include "file_ops.h"
#include "input.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define FILE_DOC "doctors.dat"
#define FILE_PAT "patients.dat"
#define FILE_APP "appointments.dat"
#define FILE_BIL "bills.dat"

void hospital_init(Hospital *hosp) {
    if (!hosp) return;
    memset(hosp, 0, sizeof(Hospital));
    hosp->next_doc_id  = 1;
    hosp->next_pat_id  = 1;
    hosp->next_appt_id = 1;
    hosp->next_bill_id = 1;

    const char *env = getenv("DATA_DIR");
    if (env && env[0] != '\0') {
        snprintf(hosp->data_dir, sizeof(hosp->data_dir), "%s", env);
    } else {
        snprintf(hosp->data_dir, sizeof(hosp->data_dir), "./data");
    }
}

void hospital_free(Hospital *hosp) {
    if (!hosp) return;
    free(hosp->doctors);
    free(hosp->patients);
    free(hosp->appointments);
    free(hosp->bills);
    memset(hosp, 0, sizeof(Hospital));
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

bool hospital_load(Hospital *hosp) {
    if (!hosp) return false;
    if (!util_ensure_dir(hosp->data_dir)) return false;

    void *data;
    uint32_t count, next_id;

    /* Doctors */
    load_records(hosp->data_dir, FILE_DOC, HOS_DOC_MAGIC, &data, &count, &next_id, sizeof(Doctor));
    hosp->doctors      = (Doctor *)data;
    hosp->doc_count    = (int)count;
    hosp->doc_capacity = (int)count;
    hosp->next_doc_id  = (int)next_id;

    /* Patients */
    load_records(hosp->data_dir, FILE_PAT, HOS_PAT_MAGIC, &data, &count, &next_id, sizeof(Patient));
    hosp->patients     = (Patient *)data;
    hosp->pat_count    = (int)count;
    hosp->pat_capacity = (int)count;
    hosp->next_pat_id  = (int)next_id;

    /* Appointments */
    load_records(hosp->data_dir, FILE_APP, HOS_APP_MAGIC, &data, &count, &next_id, sizeof(Appointment));
    hosp->appointments  = (Appointment *)data;
    hosp->appt_count    = (int)count;
    hosp->appt_capacity = (int)count;
    hosp->next_appt_id  = (int)next_id;

    /* Bills */
    load_records(hosp->data_dir, FILE_BIL, HOS_BIL_MAGIC, &data, &count, &next_id, sizeof(Bill));
    hosp->bills         = (Bill *)data;
    hosp->bill_count    = (int)count;
    hosp->bill_capacity = (int)count;
    hosp->next_bill_id  = (int)next_id;

    return true;
}

bool hospital_save(Hospital *hosp) {
    if (!hosp) return false;
    char path[512];

    snprintf(path, sizeof(path), "%s/%s", hosp->data_dir, FILE_DOC);
    file_atomic_save(path, HOS_DOC_MAGIC, hosp->doctors, sizeof(Doctor),
                     (uint32_t)hosp->doc_count, (uint32_t)hosp->next_doc_id);

    snprintf(path, sizeof(path), "%s/%s", hosp->data_dir, FILE_PAT);
    file_atomic_save(path, HOS_PAT_MAGIC, hosp->patients, sizeof(Patient),
                     (uint32_t)hosp->pat_count, (uint32_t)hosp->next_pat_id);

    snprintf(path, sizeof(path), "%s/%s", hosp->data_dir, FILE_APP);
    file_atomic_save(path, HOS_APP_MAGIC, hosp->appointments, sizeof(Appointment),
                     (uint32_t)hosp->appt_count, (uint32_t)hosp->next_appt_id);

    snprintf(path, sizeof(path), "%s/%s", hosp->data_dir, FILE_BIL);
    file_atomic_save(path, HOS_BIL_MAGIC, hosp->bills, sizeof(Bill),
                     (uint32_t)hosp->bill_count, (uint32_t)hosp->next_bill_id);

    return true;
}

/* ------------------------------------------------------------------ */
/* Doctors                                                            */
/* ------------------------------------------------------------------ */

int doctor_add(Hospital *hosp, const char *name, const char *spec, const char *phone) {
    if (!hosp || !name || name[0] == '\0') return -1;

    if (hosp->doc_count >= hosp->doc_capacity) {
        int new_cap = (hosp->doc_capacity == 0) ? 8 : hosp->doc_capacity * 2;
        Doctor *grown = realloc(hosp->doctors, (size_t)new_cap * sizeof(Doctor));
        if (!grown) return -1;
        hosp->doctors = grown;
        hosp->doc_capacity = new_cap;
    }

    Doctor *d = &hosp->doctors[hosp->doc_count++];
    memset(d, 0, sizeof(Doctor));
    d->id = hosp->next_doc_id++;
    strncpy(d->name, name, sizeof(d->name) - 1);
    if (spec) strncpy(d->specialization, spec, sizeof(d->specialization) - 1);
    if (phone) strncpy(d->phone, phone, sizeof(d->phone) - 1);
    d->is_deleted = 0;

    return d->id;
}

Doctor *doctor_find_by_id(Hospital *hosp, int id) {
    if (!hosp || id <= 0) return NULL;
    for (int i = 0; i < hosp->doc_count; i++) {
        if (hosp->doctors[i].id == id && !hosp->doctors[i].is_deleted) {
            return &hosp->doctors[i];
        }
    }
    return NULL;
}

bool doctor_update(Hospital *hosp, int id, const char *name, const char *spec, const char *phone) {
    Doctor *d = doctor_find_by_id(hosp, id);
    if (!d) return false;
    if (name && name[0]) strncpy(d->name, name, sizeof(d->name) - 1);
    if (spec && spec[0]) strncpy(d->specialization, spec, sizeof(d->specialization) - 1);
    if (phone && phone[0]) strncpy(d->phone, phone, sizeof(d->phone) - 1);
    return true;
}

bool doctor_delete(Hospital *hosp, int id) {
    Doctor *d = doctor_find_by_id(hosp, id);
    if (!d) return false;
    d->is_deleted = 1;
    return true;
}

void doctor_list_all(const Hospital *hosp) {
    if (!hosp) return;
    printf("\n");
    util_print_separator(70);
    printf("  %-4s | %-25s | %-20s | %s\n", "ID", "Doctor Name", "Specialization", "Phone");
    util_print_separator(70);

    int count = 0;
    for (int i = 0; i < hosp->doc_count; i++) {
        if (hosp->doctors[i].is_deleted) continue;
        printf("  %-4d | %-25.25s | %-20.20s | %s\n",
               hosp->doctors[i].id, hosp->doctors[i].name,
               hosp->doctors[i].specialization, hosp->doctors[i].phone);
        count++;
    }
    util_print_separator(70);
    printf("  Total: %d doctor(s)\n", count);
}

int doctor_search(const Hospital *hosp, const char *query, int *results, int max_results) {
    if (!hosp || !query || !results || max_results <= 0) return 0;
    int found = 0;
    for (int i = 0; i < hosp->doc_count && found < max_results; i++) {
        const Doctor *d = &hosp->doctors[i];
        if (d->is_deleted) continue;
        if (str_contains_ci(d->name, query) || str_contains_ci(d->specialization, query)) {
            results[found++] = d->id;
        }
    }
    return found;
}

/* ------------------------------------------------------------------ */
/* Patients                                                           */
/* ------------------------------------------------------------------ */

int patient_add(Hospital *hosp, const char *name, int age, char gender,
                const char *phone, const char *diag, int doc_id) {
    if (!hosp || !name || name[0] == '\0') return -1;

    if (hosp->pat_count >= hosp->pat_capacity) {
        int new_cap = (hosp->pat_capacity == 0) ? 8 : hosp->pat_capacity * 2;
        Patient *grown = realloc(hosp->patients, (size_t)new_cap * sizeof(Patient));
        if (!grown) return -1;
        hosp->patients = grown;
        hosp->pat_capacity = new_cap;
    }

    Patient *p = &hosp->patients[hosp->pat_count++];
    memset(p, 0, sizeof(Patient));
    p->id = hosp->next_pat_id++;
    strncpy(p->name, name, sizeof(p->name) - 1);
    p->age = age;
    p->gender = (gender == 'F' || gender == 'O') ? gender : 'M';
    if (phone) strncpy(p->phone, phone, sizeof(p->phone) - 1);
    if (diag) strncpy(p->diagnosis, diag, sizeof(p->diagnosis) - 1);
    p->assigned_doctor_id = doc_id;
    p->is_discharged = 0;
    p->is_deleted = 0;

    return p->id;
}

Patient *patient_find_by_id(Hospital *hosp, int id) {
    if (!hosp || id <= 0) return NULL;
    for (int i = 0; i < hosp->pat_count; i++) {
        if (hosp->patients[i].id == id && !hosp->patients[i].is_deleted) {
            return &hosp->patients[i];
        }
    }
    return NULL;
}

bool patient_discharge(Hospital *hosp, int patient_id) {
    Patient *p = patient_find_by_id(hosp, patient_id);
    if (!p) return false;
    p->is_discharged = 1;
    return true;
}

bool patient_update(Hospital *hosp, int id, const char *name, int age, char gender,
                    const char *phone, const char *diag, int doc_id) {
    Patient *p = patient_find_by_id(hosp, id);
    if (!p) return false;
    if (name && name[0]) strncpy(p->name, name, sizeof(p->name) - 1);
    if (age > 0) p->age = age;
    if (gender == 'M' || gender == 'F' || gender == 'O') p->gender = gender;
    if (phone && phone[0]) strncpy(p->phone, phone, sizeof(p->phone) - 1);
    if (diag && diag[0]) strncpy(p->diagnosis, diag, sizeof(p->diagnosis) - 1);
    if (doc_id > 0) p->assigned_doctor_id = doc_id;
    return true;
}

bool patient_delete(Hospital *hosp, int id) {
    Patient *p = patient_find_by_id(hosp, id);
    if (!p) return false;
    p->is_deleted = 1;
    return true;
}

void patient_list_all(const Hospital *hosp, bool active_only) {
    if (!hosp) return;
    printf("\n");
    util_print_separator(78);
    printf("  %-4s | %-20s | %-3s | %-1s | %-12s | %-15s | %s\n",
           "ID", "Name", "Age", "G", "Status", "Diagnosis", "Assigned Dr");
    util_print_separator(78);

    int count = 0;
    for (int i = 0; i < hosp->pat_count; i++) {
        const Patient *p = &hosp->patients[i];
        if (p->is_deleted) continue;
        if (active_only && p->is_discharged) continue;

        const char *doc_name = "Unassigned";
        for (int d = 0; d < hosp->doc_count; d++) {
            if (hosp->doctors[d].id == p->assigned_doctor_id) {
                doc_name = hosp->doctors[d].name;
                break;
            }
        }

        printf("  %-4d | %-20.20s | %-3d | %c | %-12s | %-15.15s | %s\n",
               p->id, p->name, p->age, p->gender,
               p->is_discharged ? "Discharged" : "Admitted",
               p->diagnosis, doc_name);
        count++;
    }
    util_print_separator(78);
    printf("  Total: %d patient(s)\n", count);
}

int patient_search(const Hospital *hosp, const char *query, int *results, int max_results) {
    if (!hosp || !query || !results || max_results <= 0) return 0;
    int found = 0;
    for (int i = 0; i < hosp->pat_count && found < max_results; i++) {
        const Patient *p = &hosp->patients[i];
        if (p->is_deleted) continue;
        if (str_contains_ci(p->name, query) || str_contains_ci(p->diagnosis, query)) {
            results[found++] = p->id;
        }
    }
    return found;
}

/* ------------------------------------------------------------------ */
/* Appointments                                                       */
/* ------------------------------------------------------------------ */

int appointment_book(Hospital *hosp, int patient_id, int doctor_id, time_t date, const char *notes) {
    if (!hosp || patient_id <= 0 || doctor_id <= 0) return -1;

    if (hosp->appt_count >= hosp->appt_capacity) {
        int new_cap = (hosp->appt_capacity == 0) ? 8 : hosp->appt_capacity * 2;
        Appointment *grown = realloc(hosp->appointments, (size_t)new_cap * sizeof(Appointment));
        if (!grown) return -1;
        hosp->appointments = grown;
        hosp->appt_capacity = new_cap;
    }

    Appointment *a = &hosp->appointments[hosp->appt_count++];
    a->id = hosp->next_appt_id++;
    a->patient_id = patient_id;
    a->doctor_id  = doctor_id;
    a->date       = date;
    if (notes) strncpy(a->notes, notes, sizeof(a->notes) - 1);
    else a->notes[0] = '\0';

    return a->id;
}

void appointment_list_by_doctor(const Hospital *hosp, int doc_id) {
    if (!hosp) return;
    printf("\n  Appointments for Doctor #%d:\n", doc_id);
    for (int i = 0; i < hosp->appt_count; i++) {
        const Appointment *a = &hosp->appointments[i];
        if (a->doctor_id == doc_id) {
            char dstr[32];
            struct tm *tm_info = localtime(&a->date);
            strftime(dstr, sizeof(dstr), "%Y-%m-%d %H:%M", tm_info);
            printf("    Appt #%d: Patient #%d on %s | Notes: %s\n",
                   a->id, a->patient_id, dstr, a->notes);
        }
    }
}

void appointment_list_by_patient(const Hospital *hosp, int pat_id) {
    if (!hosp) return;
    printf("\n  Appointments for Patient #%d:\n", pat_id);
    for (int i = 0; i < hosp->appt_count; i++) {
        const Appointment *a = &hosp->appointments[i];
        if (a->patient_id == pat_id) {
            char dstr[32];
            struct tm *tm_info = localtime(&a->date);
            strftime(dstr, sizeof(dstr), "%Y-%m-%d %H:%M", tm_info);
            printf("    Appt #%d: Doctor #%d on %s | Notes: %s\n",
                   a->id, a->doctor_id, dstr, a->notes);
        }
    }
}

/* ------------------------------------------------------------------ */
/* Billing                                                            */
/* ------------------------------------------------------------------ */

int billing_create(Hospital *hosp, int patient_id, double amount) {
    if (!hosp || patient_id <= 0 || amount <= 0.0) return -1;

    if (hosp->bill_count >= hosp->bill_capacity) {
        int new_cap = (hosp->bill_capacity == 0) ? 8 : hosp->bill_capacity * 2;
        Bill *grown = realloc(hosp->bills, (size_t)new_cap * sizeof(Bill));
        if (!grown) return -1;
        hosp->bills = grown;
        hosp->bill_capacity = new_cap;
    }

    Bill *b = &hosp->bills[hosp->bill_count++];
    b->id = hosp->next_bill_id++;
    b->patient_id = patient_id;
    b->amount     = amount;
    b->is_paid    = 0;
    b->date       = time(NULL);

    return b->id;
}

Bill *billing_find_by_id(Hospital *hosp, int id) {
    if (!hosp || id <= 0) return NULL;
    for (int i = 0; i < hosp->bill_count; i++) {
        if (hosp->bills[i].id == id) return &hosp->bills[i];
    }
    return NULL;
}

bool billing_pay(Hospital *hosp, int bill_id) {
    Bill *b = billing_find_by_id(hosp, bill_id);
    if (!b || b->is_paid) return false;
    b->is_paid = 1;
    return true;
}

void billing_list_all(const Hospital *hosp) {
    if (!hosp) return;
    printf("\n");
    util_print_separator(65);
    printf("  %-4s | %-10s | %-12s | %-10s | %s\n",
           "ID", "Patient ID", "Amount ($)", "Status", "Date");
    util_print_separator(65);

    double total = 0.0, paid_total = 0.0;
    for (int i = 0; i < hosp->bill_count; i++) {
        const Bill *b = &hosp->bills[i];
        char dstr[32];
        struct tm *tm_info = localtime(&b->date);
        strftime(dstr, sizeof(dstr), "%Y-%m-%d", tm_info);

        printf("  %-4d | %-10d | %-12.2f | %-10s | %s\n",
               b->id, b->patient_id, b->amount,
               b->is_paid ? "PAID ✅" : "PENDING ⏳", dstr);
        total += b->amount;
        if (b->is_paid) paid_total += b->amount;
    }
    util_print_separator(65);
    printf("  Total Billed: $%.2f | Total Collected: $%.2f\n", total, paid_total);
}

double billing_total_revenue(const Hospital *hosp) {
    if (!hosp) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < hosp->bill_count; i++) {
        if (hosp->bills[i].is_paid) sum += hosp->bills[i].amount;
    }
    return sum;
}
