#ifndef HOSPITAL_H
#define HOSPITAL_H

#include <stdbool.h>
#include <time.h>
#include <stdint.h>
#include <stddef.h>

#define DOC_NAME_LEN    64
#define DOC_SPEC_LEN    64
#define DOC_PHONE_LEN   16

#define PAT_NAME_LEN    64
#define PAT_PHONE_LEN   16
#define PAT_DIAG_LEN    128

#define APPT_NOTES_LEN  128

#define HOS_PAT_MAGIC   "HOSp"
#define HOS_DOC_MAGIC   "HOSd"
#define HOS_APP_MAGIC   "HOSa"
#define HOS_BIL_MAGIC   "HOSb"

typedef struct {
    int  id;
    char name[DOC_NAME_LEN];
    char specialization[DOC_SPEC_LEN];
    char phone[DOC_PHONE_LEN];
    int  is_deleted;
} Doctor;

typedef struct {
    int  id;
    char name[PAT_NAME_LEN];
    int  age;
    char gender;            /* 'M', 'F', 'O' */
    char phone[PAT_PHONE_LEN];
    char diagnosis[PAT_DIAG_LEN];
    int  assigned_doctor_id;
    int  is_discharged;     /* 0 = admitted, 1 = discharged */
    int  is_deleted;
} Patient;

typedef struct {
    int    id;
    int    patient_id;
    int    doctor_id;
    time_t date;
    char   notes[APPT_NOTES_LEN];
} Appointment;

typedef struct {
    int    id;
    int    patient_id;
    double amount;
    int    is_paid;        /* 0 = unpaid, 1 = paid */
    time_t date;
} Bill;

typedef struct {
    Doctor      *doctors;
    int          doc_count;
    int          doc_capacity;
    int          next_doc_id;

    Patient     *patients;
    int          pat_count;
    int          pat_capacity;
    int          next_pat_id;

    Appointment *appointments;
    int          appt_count;
    int          appt_capacity;
    int          next_appt_id;

    Bill        *bills;
    int          bill_count;
    int          bill_capacity;
    int          next_bill_id;

    char         data_dir[256];
} Hospital;

/* Lifecycle */
void hospital_init(Hospital *hosp);
void hospital_free(Hospital *hosp);
bool hospital_load(Hospital *hosp);
bool hospital_save(Hospital *hosp);

/* Doctor operations */
int     doctor_add(Hospital *hosp, const char *name, const char *spec, const char *phone);
Doctor *doctor_find_by_id(Hospital *hosp, int id);
bool    doctor_update(Hospital *hosp, int id, const char *name, const char *spec, const char *phone);
bool    doctor_delete(Hospital *hosp, int id);
void    doctor_list_all(const Hospital *hosp);
int     doctor_search(const Hospital *hosp, const char *query, int *results, int max_results);

/* Patient operations */
int      patient_add(Hospital *hosp, const char *name, int age, char gender,
                     const char *phone, const char *diag, int doc_id);
Patient *patient_find_by_id(Hospital *hosp, int id);
bool     patient_discharge(Hospital *hosp, int patient_id);
bool     patient_update(Hospital *hosp, int id, const char *name, int age, char gender,
                        const char *phone, const char *diag, int doc_id);
bool     patient_delete(Hospital *hosp, int id);
void     patient_list_all(const Hospital *hosp, bool active_only);
int      patient_search(const Hospital *hosp, const char *query, int *results, int max_results);

/* Appointment operations */
int  appointment_book(Hospital *hosp, int patient_id, int doctor_id, time_t date, const char *notes);
void appointment_list_by_doctor(const Hospital *hosp, int doc_id);
void appointment_list_by_patient(const Hospital *hosp, int pat_id);

/* Billing operations */
int    billing_create(Hospital *hosp, int patient_id, double amount);
bool   billing_pay(Hospital *hosp, int bill_id);
Bill  *billing_find_by_id(Hospital *hosp, int id);
void   billing_list_all(const Hospital *hosp);
double billing_total_revenue(const Hospital *hosp);

#endif /* HOSPITAL_H */
