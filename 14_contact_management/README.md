# Project 14: Contact Management System

A robust, production-grade contact manager demonstrating ISO C11 systems architecture, defensive format validation, duplicate prevention, crash-resilient atomic binary persistence, and RFC 4180 CSV import/export.

---

## Features

- **Contact Operations**: Add, Edit, Delete (with explicit confirmation prompt), and List.
- **Strict Format Validation**:
  - **Phone Numbers**: Enforces 7–15 digits, allowing standard separators (`+`, `-`, spaces, parentheses).
  - **Email Addresses**: Verifies single `@`, non-empty local-part, and valid domain with dot.
- **Duplicate Prevention**: Rejects duplicate contacts sharing the same normalized phone number or case-insensitive email address.
- **Search & Sort**:
  - Multi-field substring search across name, phone, email, and category.
  - Sorting by Name (Ascending A-Z), Name (Descending Z-A), and ID (Original Order).
- **RFC 4180 CSV Import & Export**:
  - Exports contact records escaping commas, quotes (`""`), and newlines.
  - Imports external CSV files with field trimming, validation, and automated duplicate skipping.
- **Crash-Resilient Persistence**:
  - Uses `safe_atomic_save()` to serialize to `data/contacts.dat` with magic bytes `CTMs`.
  - Automatically maintains `data/contacts.dat.bak`. Before updating `.bak`, the system validates the existing file to ensure a corrupted file never overwrites a valid backup.

---

## Data Structures & Storage Format

### Contact Record Layout (`#pragma pack(push, 1)`)
```c
typedef struct {
    int  id;
    char name[64];
    char phone[20];
    char email[64];
    char address[96];
    char category[32];
    int  is_deleted;
} Contact;
```
Every struct is zeroed with `memset()` before writing to disk, ensuring zero uninitialized padding bytes are persisted.

### Header Layout
```
+--------------+-----------------+--------------------+------------------+
| Magic (4B)   | Version (2B)    | Record Count (4B)  | Next ID (4B)     |
| "CTMs"       | 0x0001          | uint32_t (count)   | uint32_t (next)  |
+--------------+-----------------+--------------------+------------------+
```

---

## Build & Test Instructions

### Build Application
```bash
make
```

### Run Unit Tests & Integration Tests
```bash
make test
```
Compiles and executes test suites with AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan):
- `test_contact`: CRUD, duplicate prevention, validation, search, and sorting.
- `test_csv`: RFC 4180 escaping, quotes handling, and malformed CSV parsing.
- `test_file_integrity`: Magic verification, size checking, backup recovery, and backup preservation.
- `test_menu.sh`: Headless interactive terminal integration and clean EOF handling.

### Run Interactively
```bash
./build/contact_manager
```
