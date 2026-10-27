# Project 01: Hospital Management System

A comprehensive hospital administration and clinical record system implemented in standard C (C11). Features patient admission and discharge lifecycle, physician registry, appointment scheduling, billing and payment processing, and crash-safe binary persistence.

---

## Features

- **Patient Management**:
  - Admit patients with demographic profiles, symptoms/diagnoses, and physician assignments.
  - Track active vs. discharged status.
  - Substring search across patient records and medical diagnoses.
  - Complete discharge workflow with automated billing generation.
- **Doctor Registry**:
  - Manage doctors, medical specializations, contact information, and patient caseloads.
  - Direct linking between doctors and scheduled patient appointments.
- **Appointment Scheduling**:
  - Book patient-doctor consultations with timestamp tracking and clinical notes.
  - View appointments grouped by attending physician or patient.
- **Billing & Revenue Management**:
  - Automated invoice generation upon discharge or custom service creation.
  - Payment tracking with real-time total revenue and collected receivables metrics.
- **Crash-Safe Persistence**:
  - Dedicated binary storage (`HOSp`, `HOSd`, `HOSa`, `HOSb` magic headers) with atomic writes (`.tmp` $\to$ `fflush` $\to$ `fsync` $\to$ `.bak` $\to$ atomic `rename`).

---

## Build and Run

```bash
# Build the binary
make

# Run the hospital management system
./build/hospital

# Run unit tests and integration tests with ASan + UBSan
make test

# Clean build artifacts
make clean
```

---

## Suggested Enhancements

1. **PDF Discharge Summaries**: Integrate lightweight PDF generation for patient discharge reports.
2. **Ward & Bed Allocation**: Track hospital room and ICU bed occupancies with real-time vacancy maps.
3. **Pharmacy & Medication Inventory**: Manage hospital pharmacy stock and automatically bill prescribed medicines.
