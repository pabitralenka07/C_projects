# Project 05: Library Management System

A robust, menu-driven Library Management System implemented in standard C (C11). This project serves as the **pilot architecture** for the C projects portfolio, demonstrating robust data structures, safe input parsing, memory leak-free lifecycle management, and atomic binary file persistence with backup recovery.

---

## Features

- **Book Catalog Management**:
  - Add, list, search, view details, update, and soft-delete books.
  - Automatic ID generation and ISBN uniqueness verification.
  - Dynamic capacity expansion (`realloc` amortized growth).
  - Copy tracking: total copies and available copies.
- **Member Registry**:
  - Add, list, search, view profile, update, and soft-delete members.
  - Links directly to circulation records for viewing currently issued books.
- **Circulation (Issue & Return)**:
  - Issue books with automatic due-date calculation (14-day loan period).
  - Decrements available copies on issue; increments on return.
  - Overdue detection and automatic fine calculation ($1.00/day overdue).
  - Inspection of active loans, overdue books, and loan history by member or book.
- **Unified Multi-Field Search**:
  - Substring search across title, author, and ISBN for books.
  - Substring search across name, phone, and email for members.
- **Data Integrity & Crash Safety**:
  - Magic header validation (`LIBb`, `LIBm`, `LIBt`).
  - Atomic saves (`tmp` file → `fflush` → `fsync` → `.bak` backup → atomic `rename`).
  - Automatic detection of corrupted, truncated, or incompatible files.
  - One-click restore from `.bak` backup files.

---

## Architecture & Code Structure

```
05_library_management/
├── Makefile                # Build and test rules (supports ASan + UBSan)
├── README.md               # Project documentation & sample session
├── src/
│   ├── main.c              # Application entry point & main menu dispatch
│   ├── library.h / .c      # Central Library container, init/free lifecycle & file I/O
│   ├── book.h / .c         # Book data structure, CRUD operations & validation
│   ├── member.h / .c       # Member data structure, CRUD operations & validation
│   └── transaction.h / .c  # Issue/return circulation logic & fine calculation
└── tests/
    ├── test_book.c         # 12 unit tests for Book CRUD & ISBN constraints
    ├── test_member.c       # 9 unit tests for Member CRUD & growth
    ├── test_transaction.c  # 11 unit tests for circulation & fine calculation
    ├── test_file_integrity.c # 6 unit tests for atomic saves, corruption detection & recovery
    └── test_menu.sh        # End-to-end stdin script verifying workflows & persistence
```

Shared utilities (`../common/`) used:
- `input.h / input.c`: Safe terminal input wrapping `fgets`, with string trimming, integer bounds checking, and input buffer clearing.
- `file_ops.h / file_ops.c`: Atomic file writer and header-validated file loader with recovery routines.
- `utils.h / utils.c`: Cross-platform screen clearing and decorative table formatting.

---

## Storage & File Format

Records are stored in binary format within the data directory (`./data` or configured via the `DATA_DIR` environment variable):

```
┌──────────────────────────────────────────────┐
│ Magic Number (4 bytes): e.g. "LIBb"          │
│ Version (uint16_t): 1                        │
│ Record Count (uint32_t)                      │
│ Next Auto-Increment ID (uint32_t)            │
├──────────────────────────────────────────────┤
│ Record 1 (sizeof(Book) / Member / Txn)       │
│ Record 2 ...                                 │
│ Record N                                     │
└──────────────────────────────────────────────┘
```

---

## Build and Run

### Prerequisites
- Clang or GCC supporting C11
- Make
- Bash (for running the integration test script)

### Commands

```bash
# Build the production executable
make

# Run the application
./build/library

# Run all unit and integration tests with AddressSanitizer and UndefinedBehaviorSanitizer
make test

# Clean build artifacts and temporary data
make clean
```

---

## Sample Session Output

```
Loading library data...
  No books data file found — starting fresh.
  No members data file found — starting fresh.
  No transaction data file found — starting fresh.
Ready.

==================================================
  Library Management System
==================================================
  1. Book Management
  2. Member Management
  3. Circulation (Issue / Return)
  4. Search
  5. Exit

Choice [1-5]: 1

==================================================
  Book Management
==================================================
  1. Add Book
  2. List All Books
  3. Search Books
  4. View Book Details
  5. Update Book
  6. Delete Book
  7. Back to Main Menu

Choice [1-7]: 1

--- Add New Book ---
  Title: The C Programming Language
  Author: Brian W. Kernighan, Dennis M. Ritchie
  ISBN (Enter to skip): 978-0131103627
  Publication Year: 1988
  Number of Copies: 5

  Book added successfully! (ID: 1)

Press Enter to continue...

Choice [1-7]: 7

Choice [1-5]: 2

==================================================
  Member Management
==================================================
  1. Add Member
  2. List All Members
  3. Search Members
  4. View Member Details
  5. Update Member
  6. Delete Member
  7. Back to Main Menu

Choice [1-7]: 1

--- Add New Member ---
  Name: Ada Lovelace
  Phone: 555-0100
  Email (Enter to skip): ada@analytical.engine

  Member added successfully! (ID: 1)

Press Enter to continue...

Choice [1-7]: 7

Choice [1-5]: 3

==================================================
  Circulation (Issue / Return)
==================================================
  1. Issue a Book
  2. Return a Book
  3. View Active Issues
  4. View Overdue Books
  5. View Issues by Member
  6. View Issues by Book
  7. Back to Main Menu

Choice [1-7]: 1

--- Issue a Book ---
  Book ID: 1
  Member ID: 1

  Book issued! Transaction ID: 1, Due: 2026-11-09

Press Enter to continue...

Choice [1-7]: 7

Choice [1-5]: 5

Saving data...
Data saved successfully. Goodbye!
```

---

## Suggested Enhancements

1. **Multi-Branch Support**: Support multiple physical library locations and transfer copies between branches.
2. **Book Reservation Queue**: Allow members to join a waitlist when all copies of a book are currently on loan.
3. **CSV Export/Import**: Provide human-readable CSV reporting and bulk catalog import capabilities.
