# C Language Production Portfolio (16 Systems Projects)

[![C Standard](https://img.shields.io/badge/Standard-C11%20(ISO%2FIEC%209899%3A2011)-00599C?style=for-the-badge&logo=c)](https://github.com/pabitralenka07/C_projects)
[![Build Status](https://img.shields.io/badge/Build-Passing%20(16%2F16)-2ea44f?style=for-the-badge&logo=githubactions)](https://github.com/pabitralenka07/C_projects)
[![Sanitizers](https://img.shields.io/badge/Sanitizers-ASan%20%2B%20UBSan%20Clean-brightgreen?style=for-the-badge)](https://github.com/pabitralenka07/C_projects)
[![Compiler Warnings](https://img.shields.io/badge/Warnings-Zero%20(-Wall%20-Wextra%20-Wpedantic)-success?style=for-the-badge)](https://github.com/pabitralenka07/C_projects)
[![License](https://img.shields.io/badge/License-MIT-blue?style=for-the-badge)](https://opensource.org/licenses/MIT)

A comprehensive, production-grade systems programming portfolio consisting of **16 distinct C projects**. Engineered to strict **C11 standards**, compiling with **zero compiler warnings** under `-Wall -Wextra -Wpedantic -std=c11 -D_POSIX_C_SOURCE=200809L`, and verified against memory corruption and undefined behavior via **AddressSanitizer (ASan)**, **UndefinedBehaviorSanitizer (UBSan)**, and Apple Silicon leak analysis (`/usr/bin/leaks --atExit`).

---

## 📑 Portfolio Matrix

| # | Project | Domain | Key Architecture & Algorithms | Magic Bytes | Test Suite | Status |
|:---:|---|---|---|:---:|:---:|:---:|
| **01** | [Hospital Management](./01_hospital_management) | Healthcare Systems | Dynamic doctor-patient records, admission, discharge billing, and department revenue tracking. | `HOSp`<br>`HOSd`<br>`HOSa`<br>`HOSb` | Unit + Integration | ![Passing](https://img.shields.io/badge/100%25-Passing-brightgreen?style=flat-square) |
| **02** | [Bus Reservation](./02_bus_reservation) | Transit Logistics | Interactive 2+2 visual seat map rendering, ticket reservations, cancellations, and boarding passes. | `BUSb`<br>`BUSk` | Unit + Integration | ![Passing](https://img.shields.io/badge/100%25-Passing-brightgreen?style=flat-square) |
| **03** | [Airport Management](./03_airport_management) | Aviation Systems | Terminal flight status board (FIDS), gate assignments, passenger check-in, and flight manifests. | `AIRf`<br>`AIRp` | Unit + Integration | ![Passing](https://img.shields.io/badge/100%25-Passing-brightgreen?style=flat-square) |
| **04** | [Zoo Management](./04_zoo_management) | Operations | Enclosure capacity caps, animal feeding schedules, habitat transfers, and tiered ticket revenue. | `ZOOa`<br>`ZOOe`<br>`ZOOs`<br>`ZOOt` | Unit + Integration | ![Passing](https://img.shields.io/badge/100%25-Passing-brightgreen?style=flat-square) |
| **05** | [Library Management](./05_library_management) | Catalog & Circulation | **Pilot Architecture**: ISBN uniqueness index, circulation desk, overdue fine engine, backup recovery. | `LIBb`<br>`LIBm`<br>`LIBt` | 4 Unit Suites + Integration | ![Passing](https://img.shields.io/badge/100%25-Passing-brightgreen?style=flat-square) |
| **06** | [Bank Management](./06_bank_management) | Financial Services | Salted PIN security, 3-attempt lockouts, deposits, withdrawals, atomic wire transfers, statements. | `BNKa`<br>`BNKt` | Unit + Integration | ![Passing](https://img.shields.io/badge/100%25-Passing-brightgreen?style=flat-square) |
| **07** | [Simple Calculator](./07_simple_calculator) | Compilers / Parsers | Recursive-descent expression parser (`+`, `-`, `*`, `/`, `%`, `^`, `()`), error handling, memory & history. | N/A | Unit + Integration | ![Passing](https://img.shields.io/badge/100%25-Passing-brightgreen?style=flat-square) |
| **08** | [Palindrome Checker](./08_palindrome_checker) | String Algorithms | Strict & sentence modes, two-pointer scan, integer arithmetic check, longest palindromic substring. | N/A | Unit + Integration | ![Passing](https://img.shields.io/badge/100%25-Passing-brightgreen?style=flat-square) |
| **09** | [Factorial Calculator](./09_factorial_calculator) | Numerical Analysis | 64-bit integer factorials, arbitrary-precision big-int ($n \le 1000$), $P(n, r)$, $C(n, r)$, Stirling's approximation. | N/A | Unit + Integration | ![Passing](https://img.shields.io/badge/100%25-Passing-brightgreen?style=flat-square) |
| **10** | [To-Do List](./10_todo_list) | Productivity | Prioritized task management, categories, status toggles, multi-filter search, atomic binary saves. | `TDOi` | Unit + Integration | ![Passing](https://img.shields.io/badge/100%25-Passing-brightgreen?style=flat-square) |
| **11** | [Rock Paper Scissors](./11_rock_paper_scissors) | Game Theory | Best-of-N match play, deterministic seeded RNG, running streaks, lifetime win rates, atomic persistence. | `RPSs` | Unit + Integration | ![Passing](https://img.shields.io/badge/100%25-Passing-brightgreen?style=flat-square) |
| **12** | [Currency Converter](./12_currency_converter) | FinTech / FX | Graph/pivot exchange rates, safe double bounds checking (NaN/inf rejection), live rate overrides, inverse pairs. | `FXRt` | Unit + Integration | ![Passing](https://img.shields.io/badge/100%25-Passing-brightgreen?style=flat-square) |
| **13** | [ATM System](./13_atm_system) | Banking Hardware | Exact integer cent arithmetic (zero float drift), $20 denomination dispensing, 3-strike card lockout, audit statements. | `ATMc` | Unit + Integration | ![Passing](https://img.shields.io/badge/100%25-Passing-brightgreen?style=flat-square) |
| **14** | [Contact Management](./14_contact_management) | CRM Systems | **Pilot Extension Architecture**: RFC 4180 CSV engine, duplicate detection, case-insensitive search, atomic I/O. | `CTMs` | 3 Unit Suites + Integration | ![Passing](https://img.shields.io/badge/100%25-Passing-brightgreen?style=flat-square) |
| **15** | [Dictionary App](./15_dictionary_app) | Lexicography / Search | Case-insensitive binary search, word-of-the-day deterministic hash, curated seed loader, prefix & substring scan. | `DICT` | Unit + Integration | ![Passing](https://img.shields.io/badge/100%25-Passing-brightgreen?style=flat-square) |
| **16** | [Tic-Tac-Toe](./16_tic_tac_toe) | Game AI | Minimax adversarial search (unbeatable AI), Easy RNG mode, Pass & Play, multi-round session scoreboard. | `TTTs` | Unit + Integration | ![Passing](https://img.shields.io/badge/100%25-Passing-brightgreen?style=flat-square) |

---

## 🏛️ Architectural Highlights

```text
                                  APPLICATIONS (01 - 16)
                                            │
         ┌──────────────────┬───────────────┴───────────────┬──────────────────┐
         ▼                  ▼                               ▼                  ▼
┌─────────────────┐┌─────────────────┐            ┌─────────────────┐┌─────────────────┐
│  common/input   ││common/safe_utils│            │ common/file_ops ││   common/hash   │
├─────────────────┤├─────────────────┤            ├─────────────────┤├─────────────────┤
│ • Bounds-checked││ • TTY screen clr│            │ • Atomic writes ││ • Hex salt gen  │
│ • fgets wrapper ││ • NaN/inf reject│            │ • fflush + fsync││ • SHA-style hash│
│ • Overflow drain││ • EOF safe bool │            │ • Magic headers ││ • Constant-time │
│ • No scanf("%s")││ • Healthy backup│            │ • Auto .bak swap││   verification  │
└─────────────────┘└─────────────────┘            └─────────────────┘└─────────────────┘
```

### 1. Robust Terminal I/O & Safe Utilities (`common/input.c`, `common/safe_utils.c`)
- **Zero Format Vulnerabilities**: Complete elimination of `scanf("%s")`, buffer overflows, and format string vulnerabilities.
- **Buffer Drainage**: Bounds-checked `fgets` wrappers drain stream overflow when inputs exceed buffer sizes.
- **Strict Float & Number Validation**: `safe_read_double()` strictly checks `errno == ERANGE` and `!(val >= min && val <= max)`, rejecting `NaN`, `+inf`, `-inf`, and non-numeric garbage.
- **TTY Detection**: `safe_clear_screen()` checks `isatty(fileno(stdout))` to emit ANSI escape codes only in interactive terminals, ensuring piped/redirected test logs remain completely clean.
- **Safe Confirmation Handling**: `safe_confirm()` evaluates stream termination or `EOF` strictly to `false` (safe cancellation).

### 2. Crash-Resilient Atomic Persistence (`common/file_ops.c`, `common/safe_utils.c`)
- **Write-Ahead Temporary File**: Flushes new state to `<path>.tmp` first.
- **Physical Sync**: Calls `fflush()` and POSIX `fsync()` to force OS page write-back to physical disk.
- **Healthy Backup Guard**: Verifies existing file health before copying to `<path>.bak`. Corrupted runtime files are never permitted to overwrite good backups.
- **Atomic Swap**: POSIX `rename()` updates directory inodes atomically with zero window of data corruption.
- **Header & Schema Validation**: Verifies 4-byte packed magic signature, schema version, and byte-size match against `stat.st_size`.
- **Zero Padding Leaks**: Strict `memset(..., 0, ...)` prior to populating structs guarantees no uninitialized heap/stack memory bytes leak to disk.

### 3. Salted Cryptographic Security (`common/hash.c`)
- Used for authentication in Banking (`06`) and ATM hardware (`13`).
- 16-hex pseudorandom salt generation from system entropy.
- Constant-time string verification (`diff |= (a[i] ^ b[i])`) to neutralize timing-attack side channels.

### 4. Memory Safety & Sanitizers
- All test suites compile with AddressSanitizer (`-fsanitize=address`) and UndefinedBehaviorSanitizer (`-fsanitize=undefined`).
- Verified **0 memory leaks** via macOS Apple Silicon `/usr/bin/leaks --atExit`.
- Zero buffer overflows, out-of-bounds array reads, or signed integer overflows.

---

## ⚡ Quick Start

### Prerequisites
- **Compiler**: `gcc` or `clang` (with C11 support)
- **Build System**: GNU `make`
- **Shell**: `bash` 4.0+ (for automated test harnesses)

### 1. Build and Test All 16 Projects (One Command)
Run the unified automation engine:
```bash
./run_all.sh
```
*Live Scorecard Output:*
```text
======================================================================
                        PORTFOLIO TEST SCORECARD                      
======================================================================
No.  | Project Name                     | Duration   | Status    
-----+----------------------------------+------------+------------
1    | 01 Hospital Management           | 0s         | ✓ PASS  
2    | 02 Bus Reservation               | 0s         | ✓ PASS  
3    | 03 Airport Management            | 0s         | ✓ PASS  
4    | 04 Zoo Management                | 0s         | ✓ PASS  
5    | 05 Library Management            | 1s         | ✓ PASS  
6    | 06 Bank Management               | 0s         | ✓ PASS  
7    | 07 Simple Calculator             | 0s         | ✓ PASS  
8    | 08 Palindrome Checker            | 0s         | ✓ PASS  
9    | 09 Factorial Calculator          | 0s         | ✓ PASS  
10   | 10 To-Do List                    | 0s         | ✓ PASS  
11   | 11 Rock Paper Scissors           | 0s         | ✓ PASS  
12   | 12 Currency Converter            | 0s         | ✓ PASS  
13   | 13 ATM System                    | 0s         | ✓ PASS  
14   | 14 Contact Management            | 0s         | ✓ PASS  
15   | 15 Dictionary App                | 0s         | ✓ PASS  
16   | 16 Tic-Tac-Toe                   | 1s         | ✓ PASS  
-----+----------------------------------+------------+------------
Total Duration: 2s | Passed: 16/16

🎉 SUCCESS: All 16 C Projects verified with ZERO errors and ZERO sanitizer leaks!
```

---

## 🎮 Running Applications Interactively

Launch any project by number (1–16) or keyword using `./run_all.sh --run <target>`:

```bash
./run_all.sh --run 11           # Launches Rock Paper Scissors
./run_all.sh --run converter    # Launches Currency Converter
./run_all.sh --run atm          # Launches ATM System
./run_all.sh --run contact      # Launches Contact Management System
./run_all.sh --run dict         # Launches Dictionary App
./run_all.sh --run tictactoe    # Launches Tic-Tac-Toe
```

Or run directly from each project directory:
```bash
# Example: ATM System
cd 13_atm_system && ./build/atm
# (Default Card: 4000123456789010 | PIN: 1234)

# Example: Tic-Tac-Toe
cd 16_tic_tac_toe && ./build/tictactoe
```

---

## 🧪 Testing & Verification

```bash
# Run all unit tests (with ASan + UBSan) and integration tests across all 16 projects
make test

# Run unit tests only
./run_all.sh --unit

# Run headless menu integration tests only
./run_all.sh --integration

# Execute automated headless demos across all 16 applications
./run_all.sh --demo

# Clean all build artifacts
make clean
```

### Apple Silicon Memory Leak Check
```bash
for bin in 11_rock_paper_scissors/build/rps \
           12_currency_converter/build/converter \
           13_atm_system/build/atm \
           14_contact_management/build/contact_manager \
           15_dictionary_app/build/dict \
           16_tic_tac_toe/build/tictactoe; do
    /usr/bin/leaks --atExit -- "$bin" < /dev/null | grep -E "leaks for"
done
```
*Output:*
```text
Process: 0 leaks for 0 total leaked bytes.
```

---

## 📂 Repository Structure

```text
.
├── Makefile                     # Root build coordinator (all 16 projects)
├── README.md                    # Portfolio overview, matrix & documentation
├── plan.md                      # Master architecture plan and technical roadmap
├── walkthrough.md               # Step-by-step interactive walkthrough & user guide
├── run_all.sh                   # Unified build, test & runner automation CLI
│
├── common/                      # Shared hardened infrastructure
│   ├── file_ops.h / .c          # Atomic persistence & backup recovery
│   ├── hash.h / .c              # Salted hashing & constant-time verify
│   ├── input.h / .c             # Bounds-checked safe terminal I/O
│   ├── safe_utils.h / .c        # Safe doubles, TTY clear, safe confirms & atomic saves
│   └── utils.h / .c             # Formatting & cross-platform utils
│
├── 01_hospital_management/      # Healthcare management system
├── 02_bus_reservation/          # Transit & visual seat map booking
├── 03_airport_management/       # Flight status boards & passenger manifests
├── 04_zoo_management/           # Habitats, feeding schedules & ticketing
├── 05_library_management/       # Pilot modular library & circulation system
├── 06_bank_management/          # Salted PIN security & atomic wire transfers
├── 07_simple_calculator/        # Recursive descent arithmetic parser
├── 08_palindrome_checker/       # Two-pointer scans & numeric palindrome checks
├── 09_factorial_calculator/     # 64-bit, arbitrary big-int & Stirling engine
├── 10_todo_list/                # Priority task tracker with binary persistence
├── 11_rock_paper_scissors/      # Match play, RNG streaks & session statistics
├── 12_currency_converter/       # FX exchange rate graph & safe float conversions
├── 13_atm_system/               # Exact-cent accounting, ATM cash limits & PIN locks
├── 14_contact_management/       # Pilot CRM with RFC 4180 CSV import & export
├── 15_dictionary_app/           # Dynamic hash map, quadratic probing & word search
└── 16_tic_tac_toe/              # Unbeatable Minimax AI & match scoreboard
```

---

## 📖 Detailed Documentation

- 📘 **[Architecture Plan (`plan.md`)](./plan.md)**: Comprehensive design blueprints, invariants, binary schemas, and engineering specifications.
- 🚀 **[Technical Walkthrough (`walkthrough.md`)](./walkthrough.md)**: Step-by-step interactive tours, binary file inspection with `hexdump`, and sanitizer debugging guidelines.

---

## 📜 License

This project is open-source under the [MIT License](https://opensource.org/licenses/MIT).
