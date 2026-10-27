# C Language Portfolio: Technical Walkthrough & User Guide

This guide walks you through every layer of the **16 C Projects Portfolio**, from running individual interactive systems and inspecting binary data formats on disk to executing unit and integration test suites with AddressSanitizer and UndefinedBehaviorSanitizer enabled.

---

## Table of Contents

1. [Environment & Build Prerequisites](#1-environment--build-prerequisites)
2. [Quick Tour with `run_all.sh`](#2-quick-tour-with-run_allsh)
3. [Deep Dive: Common Infrastructure](#3-deep-dive-common-infrastructure)
   - [Safe Input System (`common/input.c`)](#safe-input-system)
   - [Atomic Persistence & Crash Safety (`common/file_ops.c`)](#atomic-persistence--crash-safety)
   - [Salted Hash & Timing-Safe Verification (`common/hash.c`)](#salted-hash--timing-safe-verification)
4. [Project-by-Project Guided Walkthrough](#4-project-by-project-guided-walkthrough)
   - [01. Hospital Management System](#01-hospital-management-system)
   - [02. Bus Reservation System](#02-bus-reservation-system)
   - [03. Airport Management System](#03-airport-management-system)
   - [04. Zoo Management System](#04-zoo-management-system)
   - [05. Library Management System](#05-library-management-system)
   - [06. Bank Management System](#06-bank-management-system)
   - [07. Simple Calculator (Recursive Descent Parser)](#07-simple-calculator)
   - [08. Palindrome Checker](#08-palindrome-checker)
   - [09. Factorial & Combinatorics Calculator](#09-factorial--combinatorics-calculator)
   - [10. To-Do List Application](#10-to-do-list-application)
   - [11. Rock Paper Scissors](#11-rock-paper-scissors)
   - [12. Currency Converter](#12-currency-converter)
   - [13. ATM System](#13-atm-system)
   - [14. Contact Management System](#14-contact-management-system)
   - [15. Dictionary Application](#15-dictionary-application)
   - [16. Tic-Tac-Toe](#16-tic-tac-toe)
5. [Inspecting On-Disk Binary Data Formats](#5-inspecting-on-disk-binary-data-formats)
6. [Debugging, Sanitizers & Memory Leak Verification](#6-debugging-sanitizers--memory-leak-verification)

---

## 1. Environment & Build Prerequisites

This portfolio compiles using standard POSIX toolchains on macOS, Linux, and Windows (via WSL or MSYS2/MinGW).

### Toolchain Requirements
- **Compiler**: `gcc` (version 7+) or `clang` (version 6+) with C11 support (`-std=c11`).
- **Build System**: GNU `make`.
- **Shell**: Bash 4.0+ for running the integration test suites.

### Global Build Commands
```bash
# Compile all 16 project binaries into their respective build/ directories
make all

# Run all unit tests (compiled with ASan + UBSan) and integration tests
make test

# Clean all compiled binaries and temporary test artifacts
make clean
```

---

## 2. Quick Tour with `run_all.sh`

The repository provides `run_all.sh`, a unified CLI manager that handles builds, tests, automated demonstrations, and interactive project execution:

```bash
# Make script executable (first time only)
chmod +x run_all.sh

# Run comprehensive test suite across all 10 projects with formatted summary
./run_all.sh

# Build all projects without running tests
./run_all.sh --build

# Run only unit test suites with AddressSanitizer and UndefinedBehaviorSanitizer
./run_all.sh --unit

# Run only headless menu integration test scripts
./run_all.sh --integration

# Launch an automated, non-interactive demo of all 10 projects
./run_all.sh --demo

# Interactively run any project (by number 1-10 or name)
./run_all.sh --run 1       # Launches Hospital Management
./run_all.sh --run bank    # Launches Bank Management
./run_all.sh --run calc    # Launches Simple Calculator
```

---

## 3. Deep Dive: Common Infrastructure

### Safe Input System
Located in `common/input.h` and `common/input.c`, this module eliminates standard C input pitfalls (`scanf("%s")`, buffer overflows, leftover newlines, trailing input garbage):

```c
/* Example usage in applications */
char name[64];
input_read_string("Enter patient name: ", name, sizeof(name));

int age;
input_read_int("Enter age (1-120): ", &age, 1, 120);

double amount;
input_read_double("Enter deposit amount: ", &amount, 0.01, 1000000.0);
```

#### How newline draining works:
When a user types more characters than fit in the buffer, `input_read_line()` detects the missing newline and immediately drains the remainder of the line from `stdin`:
```c
if (strchr(buf, '\n') == NULL) {
    int ch;
    while ((ch = fgetc(stream)) != '\n' && ch != EOF);
}
```
This guarantees that subsequent calls to `input_read_*()` do not read stale residual characters.

---

### Atomic Persistence & Crash Safety
Located in `common/file_ops.h` and `common/file_ops.c`.

When any application saves its state, it executes an atomic four-step sequence:
1. **Write Temporary File**: Data is written to `<filename>.tmp` using standard binary serialization.
2. **Kernel Flush & Sync**: `fflush()` flushes C library buffers, and `fsync()` requests the OS kernel flush dirty pages to physical non-volatile storage.
3. **Backup Snapshot**: The current valid `<filename>` is copied to `<filename>.bak`.
4. **Atomic Swap**: POSIX `rename("<filename>.tmp", "<filename>")` atomically updates the file directory entry.

#### Binary Header Validation:
Every data file starts with a packed 14-byte header:
```
+--------------+-----------------+--------------------+------------------+
| Magic (4B)   | Version (2B)    | Record Count (4B)  | Next ID (4B)     |
| e.g. "LIBb"  | 0x0001          | uint32_t (count)   | uint32_t (next)  |
+--------------+-----------------+--------------------+------------------+
```
If a file has a corrupt header or truncated payload, `file_validated_load()` detects the mismatch against `st_size` and automatically offers to restore from `<filename>.bak`.

---

### Salted Hash & Timing-Safe Verification
Located in `common/hash.h` and `common/hash.c`.

Used in [06_bank_management](file:///Users/pabitra/Downloads/C_projects/06_bank_management) for account PIN security:
- **Salt Generation**: Random 16-hex character salt generated from system entropy (`/dev/urandom` on POSIX or high-resolution clock fallback).
- **Digest Calculation**: Salted iterative bit-mixing produces a 64-character hexadecimal digest.
- **Constant-Time Verification**: Prevents timing side-channel attacks by accumulating XOR differences across all characters rather than returning early on a mismatch:
  ```c
  unsigned char diff = 0;
  for (size_t i = 0; i < len; i++) {
      diff |= (unsigned char)(a[i] ^ b[i]);
  }
  return (diff == 0);
  ```

---

## 4. Project-by-Project Guided Walkthrough

### 01. Hospital Management System
**Path**: `01_hospital_management/`  
**Binary**: `build/hospital`  
**Magic Signatures**: `HOSp` (Patients), `HOSd` (Doctors), `HOSa` (Appointments), `HOSb` (Bills)

#### Key Capabilities
- Manage Doctor profiles (name, specialty, phone).
- Register & Admit Patients with initial doctor assignment.
- Book medical appointments linking doctors and patients.
- Discharge patients and generate itemized billing invoices.
- Total hospital revenue metrics and balance calculations.

#### Try It Interactively
```bash
./run_all.sh --run 1
```
*Sample Workflow*:
1. Select **2** (Doctor Management) $\to$ **1** (Add Doctor): Enter `"Dr. Alice Smith"`, Specialty: `"Cardiology"`, Phone: `"555-0101"`.
2. Select **5** (Back to Main Menu).
3. Select **1** (Patient Management) $\to$ **1** (Admit Patient): Enter `"John Doe"`, Age: `42`, Gender: `M`, Phone: `"555-0199"`, Diagnosis: `"Arrhythmia"`, Doctor ID: `1`.
4. Select **5** (Discharge Patient): Enter Patient ID `1`, enter invoice amount: `450.00`.
5. Select **4** (Billing Menu) $\to$ **2** (Settle Bill): Enter Bill ID `1`.
6. Select **3** (Financial Summary): View collected revenue.

---

### 02. Bus Reservation System
**Path**: `02_bus_reservation/`  
**Binary**: `build/bus`  
**Magic Signatures**: `BUSb` (Buses), `BUSk` (Bookings)

#### Key Capabilities
- Fleet management: add bus routes with departure time, total seats (up to 60), and ticket fare.
- Visual 2+2 seat map rendering showing available (`[01:Free]`) vs occupied (`[02:BUSY]`) seats.
- Seat booking with passenger contact details and automated ticket boarding pass generation.
- Ticket cancellations with automatic seat liberation and fare audit tracking.

#### Try It Interactively
```bash
./run_all.sh --run 2
```
*Visual Seat Map Example*:
```text
=== Seat Map for Bus #1 (Express Shuttle) ===
  [01:BUSY] [02:Free]    ||    [03:Free] [04:Free]
  [05:Free] [06:Free]    ||    [07:BUSY] [08:Free]
Available Seats: 38 / 40 | Fare: $35.00
```

---

### 03. Airport Management System
**Path**: `03_airport_management/`  
**Binary**: `build/airport`  
**Magic Signatures**: `AIRf` (Flights), `AIRp` (Passengers)

#### Key Capabilities
- Terminal flight information display board (FIDS) showing flight number, airline, destination, departure time, gate, and status.
- Flight state transitions: `"On Time"`, `"Boarding"`, `"Departed"`, `"Delayed"`, `"Cancelled"`.
- Passenger booking and gate check-in with printed boarding passes.
- Complete flight passenger manifests.

#### Try It Interactively
```bash
./run_all.sh --run 3
```
*Sample Flight Board*:
```text
+--------+------------------+-------------+-------------+----------+------+------------+
| Flight | Airline          | Origin      | Destination | Time     | Gate | Status     |
+--------+------------------+-------------+-------------+----------+------+------------+
| BA-212 | British Airways  | London LHR  | New York JFK| 14:30    | 12   | Boarding   |
| AF-305 | Air France       | Paris CDG   | Tokyo NRT   | 16:15    | 04   | On Time    |
+--------+------------------+-------------+-------------+----------+------+------------+
```

---

### 04. Zoo Management System
**Path**: `04_zoo_management/`  
**Binary**: `build/zoo`  
**Magic Signatures**: `ZOOa` (Animals), `ZOOe` (Enclosures), `ZOOs` (Staff), `ZOOt` (Tickets)

#### Key Capabilities
- Habitat enclosures with capacity capping (Aviary, Aquarium, Safari, Reptile House).
- Animal registration with species, age, enclosure placement, and feeding schedules.
- Animal habitat transfer with strict capacity enforcement (rejects moves if target habitat is full).
- Staff member management with assigned habitat zones.
- Ticketing gate with categorized admission (Adult $20, Child $12, Senior $15) and daily revenue reports.

#### Try It Interactively
```bash
./run_all.sh --run 4
```

---

### 05. Library Management System
**Path**: `05_library_management/`  
**Binary**: `build/library`  
**Magic Signatures**: `LIBb` (Books), `LIBm` (Members), `LIBt` (Transactions)

#### Key Capabilities
- Catalog management: add books with ISBN, title, author, year, and quantity.
- ISBN uniqueness verification on input.
- Circulation desk: checkout (issue) books to registered members, validating availability.
- Return processing with automated overdue fine calculation engine ($0.50/day overdue).
- Multi-field substring search across title, author, or ISBN.
- Soft deletion preserving checkout history.

#### Try It Interactively
```bash
./run_all.sh --run 5
```

---

### 06. Bank Management System
**Path**: `06_bank_management/`  
**Binary**: `build/bank`  
**Magic Signatures**: `BNKa` (Accounts), `BNKt` (Transactions)

#### Key Capabilities
- Account creation with random 16-hex salt generation and SHA-style PIN digest.
- Three-strike failed PIN lockout protection with administrative override unlock.
- Cash deposits and withdrawals with balance checks.
- Inter-account wire transfers executed atomically across source and destination records.
- Account statements displaying full historical transaction ledger.

#### Try It Interactively
```bash
./run_all.sh --run 6
```
*Lockout Demonstration*:
1. Choose an existing account and intentionally enter an incorrect PIN 3 times.
2. Observe lockout alert: `ALERT: Account #1001 has been LOCKED! (3 failed attempts)`.
3. Subsequent operations on this account are blocked until unlocked by an administrator.

---

### 07. Simple Calculator (Recursive Descent Parser)
**Path**: `07_simple_calculator/`  
**Binary**: `build/calculator`  
**Magic Signatures**: N/A (Stateless computation with memory register)

#### Key Capabilities
- Evaluates arbitrary arithmetic expressions containing `+`, `-`, `*`, `/`, `%` (modulo), `^` (exponentiation), and nested parentheses `(`, `)`.
- Respects mathematical operator precedence and right-associative exponentiation ($2 \text{\textasciicircum} 3 \text{\textasciicircum} 2 = 512$).
- Unary negation support (`-5 + 3`, `(-2)^4`).
- Arithmetic exception handling: division by zero, modulo by zero, syntax errors, and numeric overflow.
- Memory accumulator (`M+`, `MR`, `MC`) and rolling 20-entry calculation history buffer.

#### Try It Interactively
```bash
./run_all.sh --run 7
```
*Sample Expressions*:
- `(10 + 20) * 3 / 2` $\implies$ `45.000000`
- `2 ^ 3 ^ 2` $\implies$ `512.000000`
- `100 % 7` $\implies$ `2.000000`
- `5 / 0` $\implies$ `Error: Division by zero`

---

### 08. Palindrome Checker
**Path**: `08_palindrome_checker/`  
**Binary**: `build/palindrome`  
**Magic Signatures**: N/A (Algorithmic text analysis)

#### Key Capabilities
- **Strict Byte-for-Byte Check**: Exact reverse character equality.
- **Sentence / Relaxed Mode**: Two-pointer scan ignoring casing, spaces, and punctuation (`"A man, a plan, a canal: Panama!"` $\implies$ Valid Palindrome).
- **Pure Numeric Check**: Arithmetic reversal of integer digits without allocating string buffers or converting to text.
- **Longest Palindromic Substring**: $O(N^2)$ expansion algorithm finding the maximum palindromic segment inside any arbitrary string (e.g. `"babad"` $\implies$ `"bab"` or `"aba"`).

#### Try It Interactively
```bash
./run_all.sh --run 8
```

---

### 09. Factorial & Combinatorics Calculator
**Path**: `09_factorial_calculator/`  
**Binary**: `build/factorial`  
**Magic Signatures**: N/A (Mathematical computation)

#### Key Capabilities
- **Standard 64-Bit Factorial**: Fast hardware integer multiplication up to $20! = 2,432,902,008,176,640,000$.
- **Big-Integer Exact Engine**: Dynamic base-10 digit array computing exact factorials for $n \le 1000$ (e.g. $100!$ has 158 digits; $1000!$ has 2568 digits).
- **Permutations & Combinations**:
  $$P(n, r) = \frac{n!}{(n - r)!}, \quad C(n, r) = \binom{n}{r} = \frac{n!}{r!(n - r)!}$$
  Calculates poker hands, lottery probabilities, and permutations with overflow checks.
- **Stirling's Asymptotic Approximation**: High-speed estimation of large factorials using Stirling's series.

#### Try It Interactively
```bash
./run_all.sh --run 9
```

---

### 10. To-Do List Application
**Path**: `10_todo_list/`  
**Binary**: `build/todo`  
**Magic Signatures**: `TDOi`

#### Key Capabilities
- Task management: title, category tags (e.g. "Work", "Personal", "Health"), priority levels (Low, Medium, High).
- Completion toggling: marks items as Pending `[ ]` or Completed `[X]`.
- Filtering views: display all, only pending, only completed, or filter by minimum priority level.
- Atomic binary persistence ensuring task list survives process termination.

#### Try It Interactively
```bash
./run_all.sh --run 10
```

---

## 5. Inspecting On-Disk Binary Data Formats

Because the applications use structured binary files with magic headers, you can inspect their internal layout using `hexdump` or `xxd`:

```bash
# Example: Inspecting Bank Accounts binary data file
hexdump -C 06_bank_management/data/accounts.dat | head -n 10
```

### Decoded Header Layout
```text
00000000  42 4e 4b 61 01 00 03 00  00 00 04 00 00 00 e9 03  |BNKa............|
```
- Bytes 0-3: `42 4e 4b 61` $\implies$ ASCII `"BNKa"` (Magic Byte identifier).
- Bytes 4-5: `01 00` $\implies$ Version `1` (uint16_t, little-endian).
- Bytes 6-9: `03 00 00 00` $\implies$ Record Count `3` (uint32_t).
- Bytes 10-13: `04 00 00 00` $\implies$ Next ID `4` (uint32_t).
- Byte 14+: First `Account` struct payload begins.

Notice also the automatically created backup file:
```bash
ls -la 06_bank_management/data/
# Output displays: accounts.dat, accounts.dat.bak, transactions.dat, transactions.dat.bak
```

---

## 6. Debugging, Sanitizers & Memory Leak Verification

### AddressSanitizer & UndefinedBehaviorSanitizer
All test binaries are compiled with `-fsanitize=address,undefined`. If any memory corruption, out-of-bounds pointer dereference, use-after-free, or integer undefined behavior occurs, the runtime will immediately halt and print a full backtrace.

To build the main binaries themselves with sanitizers enabled:
```bash
make clean
make all CFLAGS="-Wall -Wextra -Wpedantic -std=c11 -fsanitize=address,undefined"
```

### macOS Apple Silicon Memory Leak Verification
On macOS Apple Silicon (ARM64), Clang does not support `-fsanitize=leak`. Real memory leak verification is performed using the native macOS `/usr/bin/leaks` tool at exit:
```bash
# Verify zero leaks across all new project binaries
/usr/bin/leaks --atExit -- ./11_rock_paper_scissors/build/rps < /dev/null
/usr/bin/leaks --atExit -- ./12_currency_converter/build/converter < /dev/null
/usr/bin/leaks --atExit -- ./13_atm_system/build/atm < /dev/null
/usr/bin/leaks --atExit -- ./14_contact_management/build/contact_manager < /dev/null
/usr/bin/leaks --atExit -- ./15_dictionary_app/build/dict < /dev/null
/usr/bin/leaks --atExit -- ./16_tic_tac_toe/build/tictactoe < /dev/null
```
All binaries report: `0 leaks for 0 total leaked bytes`.

### Running with Valgrind (Linux / WSL)
If testing on a Linux platform with Valgrind installed:
```bash
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./05_library_management/build/test_book
```

All 16 project test suites are verified clean with **0 memory leaks** and **0 sanitizer violations**.
