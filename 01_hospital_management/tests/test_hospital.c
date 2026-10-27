#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "hospital.h"

static void make_temp_dir(char *buf, size_t size) {
    char templ[] = "/tmp/hosp_test_XXXXXX";
    char *dir = mkdtemp(templ);
    assert(dir != NULL);
    snprintf(buf, size, "%s", dir);
}

static void cleanup_temp_dir(const char *dir) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", dir);
    (void)system(cmd);
}

static void test_doctors(void) {
    Hospital h;
    hospital_init(&h);

    int d1 = doctor_add(&h, "Gregory House", "Diagnostics", "555-0101");
    int d2 = doctor_add(&h, "James Wilson", "Oncology", "555-0102");
    assert(d1 == 1 && d2 == 2);
    assert(h.doc_count == 2);

    Doctor *doc = doctor_find_by_id(&h, 1);
    assert(doc != NULL && strcmp(doc->name, "Gregory House") == 0);

    /* Search */
    int results[10];
    int count = doctor_search(&h, "Oncology", results, 10);
    assert(count == 1 && results[0] == 2);

    /* Update */
    assert(doctor_update(&h, 1, "Dr. Gregory House", "Nephrology", "555-9999"));
    assert(strcmp(doc->specialization, "Nephrology") == 0);

    /* Soft delete */
    assert(doctor_delete(&h, 2));
    assert(doctor_find_by_id(&h, 2) == NULL);

    hospital_free(&h);
    printf("  [PASS] test_doctors\n");
}

static void test_patients(void) {
    Hospital h;
    hospital_init(&h);

    int d1 = doctor_add(&h, "Leonard McCoy", "Surgery", "555-1701");
    int p1 = patient_add(&h, "James Kirk", 35, 'M', "555-0001", "Fracture", d1);
    int p2 = patient_add(&h, "Nyota Uhura", 32, 'F', "555-0002", "Fever", d1);
    assert(p1 == 1 && p2 == 2);

    Patient *pat = patient_find_by_id(&h, 1);
    assert(pat != NULL);
    assert(pat->assigned_doctor_id == d1);
    assert(!pat->is_discharged);

    /* Discharge */
    assert(patient_discharge(&h, 1));
    assert(pat->is_discharged);

    /* Search */
    int res[10];
    int count = patient_search(&h, "Fever", res, 10);
    assert(count == 1 && res[0] == 2);

    hospital_free(&h);
    printf("  [PASS] test_patients\n");
}

static void test_appointments_and_billing(void) {
    Hospital h;
    hospital_init(&h);

    int d1 = doctor_add(&h, "Doogie Howser", "Pediatrics", "555-0003");
    int p1 = patient_add(&h, "Vinnie Delpino", 16, 'M', "555-0004", "Mild Cold", d1);

    int a1 = appointment_book(&h, p1, d1, time(NULL), "General checkup");
    assert(a1 == 1);
    assert(h.appt_count == 1);

    /* Billing */
    int b1 = billing_create(&h, p1, 150.0);
    int b2 = billing_create(&h, p1, 50.0);
    assert(b1 == 1 && b2 == 2);

    Bill *bill = billing_find_by_id(&h, 1);
    assert(bill != NULL && bill->amount == 150.0 && !bill->is_paid);

    assert(billing_pay(&h, 1));
    assert(bill->is_paid);
    assert(billing_total_revenue(&h) == 150.0);

    hospital_free(&h);
    printf("  [PASS] test_appointments_and_billing\n");
}

static void test_persistence_roundtrip(void) {
    char test_dir[256];
    make_temp_dir(test_dir, sizeof(test_dir));
    setenv("DATA_DIR", test_dir, 1);

    Hospital h1;
    hospital_init(&h1);
    hospital_load(&h1);

    int d = doctor_add(&h1, "Meredith Grey", "General Surgery", "555-2000");
    int p = patient_add(&h1, "Derek Shepherd", 40, 'M', "555-2001", "Head Trauma", d);
    appointment_book(&h1, p, d, time(NULL), "MRI Consult");
    billing_create(&h1, p, 1200.0);
    assert(hospital_save(&h1));
    hospital_free(&h1);

    Hospital h2;
    hospital_init(&h2);
    assert(hospital_load(&h2));
    assert(h2.doc_count == 1);
    assert(h2.pat_count == 1);
    assert(h2.appt_count == 1);
    assert(h2.bill_count == 1);
    assert(strcmp(h2.doctors[0].name, "Meredith Grey") == 0);
    assert(strcmp(h2.patients[0].name, "Derek Shepherd") == 0);
    assert(h2.bills[0].amount == 1200.0);

    hospital_free(&h2);
    cleanup_temp_dir(test_dir);
    printf("  [PASS] test_persistence_roundtrip\n");
}

int main(void) {
    printf("=== Running Hospital Management Unit Tests ===\n");
    test_doctors();
    test_patients();
    test_appointments_and_billing();
    test_persistence_roundtrip();
    printf("All hospital unit tests passed!\n");
    return 0;
}
