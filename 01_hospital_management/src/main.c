/*
 * Hospital Management System — Main Entry Point
 * Persisted medical records: patients, doctors, appointments,
 * discharge billing, and unified search.
 */

#include "hospital.h"
#include "input.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_main_menu(void) {
    printf("\n");
    util_print_header("Hospital Management System");
    printf("  1. Patient Management (Admit / List / Discharge)\n");
    printf("  2. Doctor Management (Add / List / Search)\n");
    printf("  3. Appointments (Schedule / View)\n");
    printf("  4. Billing & Finance\n");
    printf("  5. Unified Search\n");
    printf("  6. Exit\n\n");
}

/* ------------------------------------------------------------------ */
/* Patient Submenu                                                    */
/* ------------------------------------------------------------------ */

static void patient_menu(Hospital *hosp) {
    for (;;) {
        printf("\n");
        util_print_header("Patient Management");
        printf("  1. Admit New Patient\n");
        printf("  2. List Active (Admitted) Patients\n");
        printf("  3. List All Patients (including Discharged)\n");
        printf("  4. Search Patients\n");
        printf("  5. Discharge Patient\n");
        printf("  6. Back to Main Menu\n\n");

        int choice = input_menu_choice(1, 6);
        if (choice == -1 || choice == 6) break;

        switch (choice) {
        case 1: {
            printf("\n--- Admit New Patient ---\n");
            char name[PAT_NAME_LEN], phone[PAT_PHONE_LEN], diag[PAT_DIAG_LEN];
            int age, doc_id;
            char gbuf[16];

            if (!input_read_string("  Patient Name: ", name, sizeof(name))) break;
            if (!input_read_int("  Age: ", &age, 0, 130)) break;

            printf("  Gender (M/F/O): ");
            fflush(stdout);
            if (!input_read_line(gbuf, sizeof(gbuf), stdin)) break;
            char gender = (gbuf[0] == 'F' || gbuf[0] == 'f') ? 'F' :
                          (gbuf[0] == 'O' || gbuf[0] == 'o') ? 'O' : 'M';

            if (!input_read_string("  Phone: ", phone, sizeof(phone))) break;
            if (!input_read_string("  Diagnosis: ", diag, sizeof(diag))) break;

            doctor_list_all(hosp);
            if (!input_read_int("  Assign Doctor ID (0 for unassigned): ", &doc_id, 0, 999999)) break;

            int id = patient_add(hosp, name, age, gender, phone, diag, doc_id);
            if (id > 0) {
                printf("\n  Patient admitted successfully! (ID: %d)\n", id);
                hospital_save(hosp);
            }
            break;
        }
        case 2:
            patient_list_all(hosp, true);
            break;
        case 3:
            patient_list_all(hosp, false);
            break;
        case 4: {
            char query[64];
            if (!input_read_string("  Search query: ", query, sizeof(query))) break;
            int results[50];
            int count = patient_search(hosp, query, results, 50);
            printf("\n  Found %d matching patient(s):\n", count);
            for (int i = 0; i < count; i++) {
                Patient *p = patient_find_by_id(hosp, results[i]);
                if (p) {
                    printf("  [%d] %s (Age: %d, %s) - %s\n",
                           p->id, p->name, p->age,
                           p->is_discharged ? "Discharged" : "Admitted",
                           p->diagnosis);
                }
            }
            break;
        }
        case 5: {
            int pid;
            if (!input_read_int("  Enter Patient ID to discharge: ", &pid, 1, 999999)) break;
            Patient *p = patient_find_by_id(hosp, pid);
            if (!p) {
                printf("  Patient #%d not found.\n", pid);
                break;
            }
            if (p->is_discharged) {
                printf("  Patient is already discharged.\n");
                break;
            }

            double fee;
            if (!input_read_double("  Discharge treatment charge ($): ", &fee, 0.0, 100000.0)) break;

            patient_discharge(hosp, pid);
            int bid = billing_create(hosp, pid, fee);
            printf("\n  Patient #%d discharged! Generated Bill #%d ($%.2f).\n", pid, bid, fee);
            hospital_save(hosp);
            break;
        }
        default:
            break;
        }
        util_press_enter();
    }
}

/* ------------------------------------------------------------------ */
/* Doctor Submenu                                                     */
/* ------------------------------------------------------------------ */

static void doctor_menu(Hospital *hosp) {
    for (;;) {
        printf("\n");
        util_print_header("Doctor Management");
        printf("  1. Add Doctor\n");
        printf("  2. List All Doctors\n");
        printf("  3. Search Doctors\n");
        printf("  4. View Doctor Appointments\n");
        printf("  5. Back to Main Menu\n\n");

        int choice = input_menu_choice(1, 5);
        if (choice == -1 || choice == 5) break;

        switch (choice) {
        case 1: {
            printf("\n--- Add Doctor ---\n");
            char name[DOC_NAME_LEN], spec[DOC_SPEC_LEN], phone[DOC_PHONE_LEN];
            if (!input_read_string("  Doctor Name: ", name, sizeof(name))) break;
            if (!input_read_string("  Specialization: ", spec, sizeof(spec))) break;
            if (!input_read_string("  Phone: ", phone, sizeof(phone))) break;

            int id = doctor_add(hosp, name, spec, phone);
            if (id > 0) {
                printf("\n  Doctor added successfully! (ID: %d)\n", id);
                hospital_save(hosp);
            }
            break;
        }
        case 2:
            doctor_list_all(hosp);
            break;
        case 3: {
            char query[64];
            if (!input_read_string("  Search query: ", query, sizeof(query))) break;
            int results[50];
            int count = doctor_search(hosp, query, results, 50);
            printf("\n  Found %d matching doctor(s):\n", count);
            for (int i = 0; i < count; i++) {
                Doctor *d = doctor_find_by_id(hosp, results[i]);
                if (d) {
                    printf("  [%d] Dr. %s - %s (Phone: %s)\n",
                           d->id, d->name, d->specialization, d->phone);
                }
            }
            break;
        }
        case 4: {
            int did;
            if (!input_read_int("  Enter Doctor ID: ", &did, 1, 999999)) break;
            appointment_list_by_doctor(hosp, did);
            break;
        }
        default:
            break;
        }
        util_press_enter();
    }
}

/* ------------------------------------------------------------------ */
/* Appointments Submenu                                               */
/* ------------------------------------------------------------------ */

static void appointment_menu(Hospital *hosp) {
    for (;;) {
        printf("\n");
        util_print_header("Appointment Scheduling");
        printf("  1. Schedule New Appointment\n");
        printf("  2. View Appointments by Doctor\n");
        printf("  3. View Appointments by Patient\n");
        printf("  4. Back to Main Menu\n\n");

        int choice = input_menu_choice(1, 4);
        if (choice == -1 || choice == 4) break;

        switch (choice) {
        case 1: {
            int pid, did;
            char notes[APPT_NOTES_LEN];

            if (!input_read_int("  Patient ID: ", &pid, 1, 999999)) break;
            if (!input_read_int("  Doctor ID: ", &did, 1, 999999)) break;
            printf("  Notes (e.g. Checkup, Follow-up): ");
            fflush(stdout);
            if (!input_read_line(notes, sizeof(notes), stdin)) break;

            int aid = appointment_book(hosp, pid, did, time(NULL), notes);
            if (aid > 0) {
                printf("\n  Appointment #%d booked successfully!\n", aid);
                hospital_save(hosp);
            }
            break;
        }
        case 2: {
            int did;
            if (!input_read_int("  Doctor ID: ", &did, 1, 999999)) break;
            appointment_list_by_doctor(hosp, did);
            break;
        }
        case 3: {
            int pid;
            if (!input_read_int("  Patient ID: ", &pid, 1, 999999)) break;
            appointment_list_by_patient(hosp, pid);
            break;
        }
        default:
            break;
        }
        util_press_enter();
    }
}

/* ------------------------------------------------------------------ */
/* Billing Submenu                                                    */
/* ------------------------------------------------------------------ */

static void billing_menu(Hospital *hosp) {
    for (;;) {
        printf("\n");
        util_print_header("Billing & Finances");
        printf("  1. List All Bills & Financial Summary\n");
        printf("  2. Process Bill Payment\n");
        printf("  3. Create Custom Invoice\n");
        printf("  4. Back to Main Menu\n\n");

        int choice = input_menu_choice(1, 4);
        if (choice == -1 || choice == 4) break;

        switch (choice) {
        case 1:
            billing_list_all(hosp);
            break;
        case 2: {
            int bid;
            if (!input_read_int("  Enter Bill ID to mark as paid: ", &bid, 1, 999999)) break;
            if (billing_pay(hosp, bid)) {
                printf("\n  Payment received for Bill #%d!\n", bid);
                hospital_save(hosp);
            } else {
                printf("  Bill not found or already paid.\n");
            }
            break;
        }
        case 3: {
            int pid;
            double amt;
            if (!input_read_int("  Patient ID: ", &pid, 1, 999999)) break;
            if (!input_read_double("  Invoice Amount ($): ", &amt, 0.01, 1000000.0)) break;
            int bid = billing_create(hosp, pid, amt);
            if (bid > 0) {
                printf("\n  Invoice #%d created for $%.2f\n", bid, amt);
                hospital_save(hosp);
            }
            break;
        }
        default:
            break;
        }
        util_press_enter();
    }
}

static void search_menu(Hospital *hosp) {
    char query[64];
    printf("\n--- Unified Search ---\n");
    if (!input_read_string("  Enter name or keyword: ", query, sizeof(query))) return;

    int pat_results[20], doc_results[20];
    int pat_found = patient_search(hosp, query, pat_results, 20);
    int doc_found = doctor_search(hosp, query, doc_results, 20);

    if (pat_found == 0 && doc_found == 0) {
        printf("\n  No records matching '%s' found.\n", query);
        return;
    }

    if (pat_found > 0) {
        printf("\n  Patients (%d):\n", pat_found);
        for (int i = 0; i < pat_found; i++) {
            Patient *p = patient_find_by_id(hosp, pat_results[i]);
            if (p) printf("    [P%d] %s (%s)\n", p->id, p->name, p->diagnosis);
        }
    }

    if (doc_found > 0) {
        printf("\n  Doctors (%d):\n", doc_found);
        for (int i = 0; i < doc_found; i++) {
            Doctor *d = doctor_find_by_id(hosp, doc_results[i]);
            if (d) printf("    [D%d] Dr. %s (%s)\n", d->id, d->name, d->specialization);
        }
    }
}

int main(void) {
    Hospital hosp;
    hospital_init(&hosp);

    printf("Loading hospital records...\n");
    hospital_load(&hosp);
    printf("Ready.\n");

    for (;;) {
        print_main_menu();
        int choice = input_menu_choice(1, 6);
        if (choice == -1 || choice == 6) break;

        switch (choice) {
        case 1: patient_menu(&hosp); break;
        case 2: doctor_menu(&hosp); break;
        case 3: appointment_menu(&hosp); break;
        case 4: billing_menu(&hosp); break;
        case 5: search_menu(&hosp); util_press_enter(); break;
        default: break;
        }
    }

    printf("\nSaving hospital records...\n");
    hospital_save(&hosp);
    hospital_free(&hosp);
    printf("Goodbye!\n");
    return EXIT_SUCCESS;
}
