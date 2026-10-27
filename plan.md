# C Language Portfolio: Master Architecture Plan & Blueprint

> **Engineering Specification & Roadmap**  
> **Standard**: ISO/IEC 9899:2011 (C11)  
> **Toolchain**: GCC / Clang, Make, POSIX.1-2008  
> **Sanitizers**: AddressSanitizer (ASan), UndefinedBehaviorSanitizer (UBSan)  
> **Compilation Policy**: Zero Warnings (`-Wall -Wextra -Wpedantic -std=c11`)

---

## 1. Executive Vision & Objectives

The goal of this portfolio is to demonstrate **production-grade systems programming in ANSI/ISO C11**. Unlike introductory or academic sample programs that rely on unsafe primitives like `scanf("%s", ...)` or vulnerable in-place binary writes, every project in this portfolio adheres to rigorous industry standards:

1. **Zero-Warning Strict Conformance**: Compiles cleanly with `-Wall -Wextra -Wpedantic -std=c11` on both Clang and GCC.
2. **Defensive Input Engineering**: Complete elimination of format string attacks, integer overflow on input conversion, and buffer overflow via a dedicated `common/input` wrapper around `fgets()`, `strtol()`, and `strtod()`.
3. **Crash-Resilient Atomic Persistence**: Write-ahead temporary file generation, explicit kernel flush (`fflush`), directory/file synchronization (`fsync`), backup rotation (`.bak`), and atomic swap (`rename`) to guarantee zero file corruption even upon power loss or process kill (`SIGKILL`).
4. **Header Validation & Self-Healing Data Files**: Custom 4-byte magic identifiers, packed headers, schema versioning, file size verification, and automated recovery from secondary backup snapshots.
5. **Memory Safety & Sanitizer Cleanliness**: Zero memory leaks, zero heap-buffer-overflows, and zero undefined behavior as verified by AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan).
6. **Isolated Automated Verification**: Dual-layer test harness containing both programmatic unit tests with full branch assertions and automated headless integration test scripts verifying terminal UX.

---

## 2. Core Architectural Pillars (`common/`)

All 16 applications share a hardened foundational layer located in `common/`:

```
                       ┌────────────────────────────────────────┐
                       │          Applications (01 - 16)        │
                       └───────────────────┬────────────────────┘
                                           │
         ┌──────────────────┬──────────────┴─────┬─────────────────┐
         ▼                  ▼                    ▼                 ▼
 ┌───────────────┐  ┌───────────────┐    ┌───────────────┐ ┌───────────────┐
 │ common/input  │  │common/file_ops│    │  common/hash  │ │ common/utils  │
 ├───────────────┤  ├───────────────┤    ├───────────────┤ ├───────────────┤
 │- fgets wrapper│  │- Atomic tmp   │    │- Salt gen     │ │- ANSI screens │
 │- strtol/strtod│  │- fflush/fsync │    │- SHA-style    │ │- Table borders│
 │- Stream drain │  │- Backup swap  │    │  salted hash  │ │- util_ensure_ │
 │- Bounds check │  │- Magic bytes  │    │- Constant-time│ │  dir()        │
 │- Auto-trim    │  │- Recovery     │    │  verify       │ │- Cross-plat   │
 └───────────────┘  └───────────────┘    └───────────────┘ └───────────────┘
```

### 2.1 Safe Terminal I/O (`common/input.h`, `common/input.c`)
- **Vulnerability Mitigated**: Stack buffer overflow, newline contamination, unparsed trailing garbage, unbounded string inputs.
- **Implementation**:
  - `input_read_line(char *buf, size_t len, FILE *stream)`: Uses `fgets()`. Detects whether the newline was captured; if not, safely drains excess characters from the input stream up to `\n` or `EOF` to prevent input poisoning of subsequent prompts.
  - `input_read_int(prompt, &out, min, max)`: Wraps `strtol()`, checks for `ERANGE`, validates numeric conversion characters, and verifies bounds `[min, max]`. Re-prompts gracefully on invalid input.
  - `input_read_double(prompt, &out, min, max)`: Wraps `strtod()`, checks for `ERANGE`, `isnan`, and `isinf`.
  - `input_read_string(prompt, buf, len)`: Reads trimmed, non-empty text strings with automatic whitespace trimming.
  - `input_confirm(prompt, default_val)`: Safely accepts `y/n`, defaulting on empty `Enter`.

### 2.2 Atomic Persistence & File Integrity (`common/file_ops.h`, `common/file_ops.c`)
- **Vulnerability Mitigated**: Incomplete binary writes, power failure truncation, corrupted data files crashing the engine.
- **Binary Header Structure (`#pragma pack(push, 1)`)**:
  ```c
  typedef struct {
      char     magic[4];       /* 4-byte ASCII signature (e.g., 'LIBb', 'BNKa') */
      uint16_t version;        /* Schema version (default 1) */
      uint32_t record_count;  /* Number of payload structs in file */
      uint32_t next_id;       /* Monotonically increasing ID counter */
  } FileHeader;
  ```
- **Atomic Save Pipeline**:
  1. Open target destination `.tmp` file (`fopen(..., "wb")`).
  2. Write packed `FileHeader` followed by continuous array of records.
  3. Flush application buffers via `fflush()`.
  4. Force write to physical storage via POSIX `fsync(fileno(fp))` or platform equivalent.
  5. Close `.tmp` file.
  6. If existing data file exists, copy it to `<path>.bak` as safety net.
  7. Atomically rename `<path>.tmp` to `<path>` via POSIX `rename()`.
- **Validation on Load**:
  1. Validates 4-byte magic signature.
  2. Validates schema version (rejects unhandled newer versions).
  3. Calculates expected file size: `sizeof(FileHeader) + (record_count * record_size)`. Compares against physical `stat.st_size` to detect truncation.
  4. Automatically offers restoration from `.bak` if the main file is corrupt or truncated.

### 2.3 Salted Security & Cryptographic Hygiene (`common/hash.h`, `common/hash.c`)
- **Vulnerability Mitigated**: Plaintext storage of administrative/account PINs, timing-attack vulnerabilities.
- **Implementation**:
  - `hash_generate_salt()`: Generates 16-character pseudo-random hexadecimal salts from system entropy.
  - `hash_compute()`: Salted iterative hashing generating 64-character hex digests.
  - `hash_verify()`: Constant-time bitwise accumulator (`diff |= (a ^ b)`) preventing timing-attack side-channel leakage.

### 2.4 Terminal & OS Utilities (`common/utils.h`, `common/utils.c`)
- Platform-agnostic terminal clear (`\033[2J\033[H` on POSIX, `cls` on Windows).
- Directory initialization (`util_ensure_dir`) utilizing POSIX `mkdir(path, 0755)`.
- Standardized tabular separator banners and header lines for consistent UX.

---

## 3. Comprehensive Project Specifications

| # | Project Directory | Domain | Binary Output | Test Output | Data Signature(s) | Key Data Structures & Algorithms |
|---|---|---|---|---|---|---|
| **01** | `01_hospital_management` | Healthcare Information | `build/hospital` | `build/test_hospital` | `HOSp`, `HOSd`, `HOSa`, `HOSb` | Dynamic Patient, Doctor, Appointment, Bill models; soft deletion; linear & sub-field search; revenue calculation. |
| **02** | `02_bus_reservation` | Transit & Ticketing | `build/bus_reservation` | `build/test_bus` | `BUSb`, `BUSk` | Visual 2+2 seat matrix rendering; dynamic bus registry; seat allocation bitflags; booking cancellations. |
| **03** | `03_airport_management` | Flight Logistics | `build/airport` | `build/test_airport` | `AIRf`, `AIRp` | Flight status boards; gate management; passenger manifests; boarding pass generation; flight capacity enforcement. |
| **04** | `04_zoo_management` | Wildlife & Operations | `build/zoo` | `build/test_zoo` | `ZOOa`, `ZOOe`, `ZOOs`, `ZOOt` | Enclosure capacity validation; animal feeding schedules; staff role assignment; tiered ticket revenue accounting. |
| **05** | `05_library_management` | Information Retrieval | `build/library` | `build/test_book`, `build/test_member`, `build/test_transaction`, `build/test_file_integrity` | `LIBb`, `LIBm`, `LIBt` | Uniqueness enforcement (ISBN); circulation tracking; active transaction index; fine calculation engine; backup recovery test suite. |
| **06** | `06_bank_management` | Financial Services | `build/bank` | `build/test_bank` | `BNKa`, `BNKt` | Salted PIN hashing; 3-attempt account lockout; atomic fund transfers with source & target verification; audit statement generation. |
| **07** | `07_simple_calculator` | Compiler / Parser | `build/calculator` | `build/test_calc` | N/A | Recursive descent parsing with operator precedence (`+`, `-`, `*`, `/`, `%`, `^`, parentheses); unary negation; expression history; memory accumulator. |
| **08** | `08_palindrome_checker` | String Algorithms | `build/palindrome` | `build/test_palindrome` | N/A | In-place two-pointer algorithm; strict vs punctuation/case-insensitive modes; numeric integer reversal; $O(N^2)$ expansion-around-center longest palindromic substring. |
| **09** | `09_factorial_calculator` | Numeric Analysis | `build/factorial` | `build/test_factorial` | N/A | Fast 64-bit integer factorials ($0 \le n \le 20$); arbitrary-precision big-integer multiplication ($n \le 1000$); permutations $P(n,r)$; combinations $C(n,r)$; Stirling's asymptotic approximation. |
| **10** | `10_todo_list` | Productivity & Workflow | `build/todo` | `build/test_todo` | `TDOi` | Dynamic tasks with priority weighting; category tagging; completion toggles; soft deletion; multi-criteria filtered search. |

---

## 4. Deep-Dive Design Specifications

### Project 01: Hospital Management System
- **Entities**: `Doctor`, `Patient`, `Appointment`, `Bill`.
- **Key Invariants**:
  - A patient cannot be discharged without settling outstanding medical bills or generating an invoice.
  - Doctor appointments are bounded by valid active doctor IDs.
  - Capacity doubling: Initial capacity of 16 grows via `realloc(ptr, capacity * 2 * sizeof(T))` with null-pointer checks to prevent memory corruption.

### Project 02: Bus Reservation System
- **Seat Map Rendering**:
  - Formats bus layouts as a 2+2 aisle configuration.
  - Visual display: `[01:Free] [02:BUSY] || [03:Free] [04:Free]`.
- **Validation**:
  - Prevents double-booking of any seat index $[1 \dots \text{total\_seats}]$.
  - Reclaims seat availability immediately on booking cancellation.

### Project 03: Airport Management System
- **Flight Status Engine**:
  - State machine: `Scheduled` $\to$ `Boarding` $\to$ `Departed` / `Delayed` / `Cancelled`.
- **Passenger Allocation**:
  - Enforces `checked_in <= capacity`.
  - Rejects passenger bookings when flight capacity reaches upper limit.

### Project 04: Zoo Management System
- **Enclosure Capping**:
  - Each enclosure has a `max_capacity`. When moving or adding an animal, system rejects transfers if `current_count >= capacity`.
- **Revenue Accounting**:
  - Categorized tickets: Adult ($20.00), Child ($12.00), Senior ($15.00). Aggregated daily ticket totals.

### Project 05: Library Management System
- **Flagship Pilot Architecture**:
  - Fully decoupled entity managers: `book.c`, `member.c`, `transaction.c`.
  - ISBN uniqueness index: Duplicate ISBN detection during creation.
  - Circulating stock count: Book `issued_count` cannot exceed `total_copies`.
  - Soft deletion: Retains relational historical integrity for previous book checkout records.

### Project 06: Bank Management System
- **Security & Integrity**:
  - Accounts hold random 16-hex salt and 64-hex SHA-style digest.
  - 3-strike lockout policy: Tracks `failed_attempts`. Upon 3rd consecutive incorrect PIN, `is_locked = 1`.
  - Atomic transfer: Deducts from source, credits destination, records double audit transaction log, and saves atomically.

### Project 07: Simple Calculator (Recursive Descent Parser)
- **Grammar**:
  $$\text{Expression} \to \text{Term} \{ (+ \mid -) \text{Term} \}$$
  $$\text{Term} \to \text{Factor} \{ (* \mid / \mid \%) \text{Factor} \}$$
  $$\text{Factor} \to \text{Base} [ \text{\textasciicircum} \text{Factor} ]$$
  $$\text{Base} \to (+ \mid -) \text{Base} \mid \text{NUMBER} \mid ( \text{Expression} )$$
- Zero-division detection on `/` and `%` yielding clean `CALC_ERR_DIV_ZERO`.
- Memory accumulator (`M+`, `MR`, `MC`) and history buffer for the last 20 evaluations.

### Project 08: Palindrome Checker
- **Algorithms**:
  - Two-pointer symmetric scan ($O(N)$ time, $O(1)$ space).
  - Integer palindrome verification using arithmetic digit reversal:
    $$\text{reversed} = \text{reversed} \times 10 + (n \pmod{10})$$
    Avoids string formatting buffer overhead and prevents integer overflow.
  - Longest Palindromic Substring: Expands outward from each center (odd and even lengths) in $O(N^2)$ time with zero dynamic allocations.

### Project 09: Factorial & Combinatorics Engine
- **Three-Tier Calculation Strategy**:
  1. *Hardware 64-bit unsigned*: Immediate computation for $0 \le n \le 20$.
  2. *Big-Int Digit Array*: Array of base-10 digits representing numbers up to thousands of digits. Multiplies accumulator in-place for $n \le 1000$.
  3. *Stirling's Approximation*:
     $$n! \approx \sqrt{2 \pi n} \left( \frac{n}{e} \right)^n \left( 1 + \frac{1}{12n} \right)$$
     Provides fast logarithmic floating-point estimates.

### Project 10: To-Do List Application
- **Task Scheduling & Filtering**:
  - Priorities: Low (1), Medium (2), High (3).
  - Filtering by status (Active/Completed) and Priority level.
  - In-place task title/category editing with atomic binary persistence (`TDOi`).

### Project 11: Rock Paper Scissors
- **Game Mechanics**: Best-of-N matches (odd N), round-by-round scorekeeping, running streaks, lifetime statistics (win/loss/draw counts and percentage).
- **Seeded RNG & Testability**: Computer choice generated via configurable seeded RNG (`RPS_SEED` environment variable or dependency injection), enabling deterministic unit and integration test assertions.
- **Atomic Persistence**: Serializes lifetime player statistics to `data/rps_stats.dat` (`RPSs`) with automatic `.bak` rotation.

### Project 12: Currency Converter
- **Multi-Currency Graph**: 8 world currencies (USD, EUR, GBP, JPY, CAD, AUD, CHF, INR) indexed to base USD with direct, cross-rate, and inverse pair conversions.
- **Safe Floating-Point Processing**: Rejects `NaN`, `+inf`, `-inf`, and non-positive exchange rates using `safe_read_double()`.
- **Runtime Overrides & Table**: Live interactive exchange rate updates with tabular matrix formatting and persistence (`FXRt`).

### Project 13: ATM System
- **Exact Cent Accounting**: All balances, daily limits, and transactions represented as 64-bit integer cents (`int64_t balance_cents`), eliminating floating-point rounding drift.
- **Physical ATM Constraints**: Enforces $20 bill dispensing multiples, $1,000.00 daily withdrawal limits, and a strict 3-attempt PIN lockout policy.
- **Salted PIN Authentication**: Cardholder credentials verified with SHA-style 16-hex salted hash (`common/hash.c`) and atomic persistence (`ATMc`).

### Project 14: Contact Management System (Flagship Extension Pilot)
- **RFC 4180 CSV Engine**: Robust CSV importer and exporter with quote escaping (`""`), comma delimiters, and multiline field safety.
- **Uniqueness & Validation**: Enforces duplicate phone and email detection and strict regex-like formatting checks for names, phones, and emails.
- **Dual Persistence**: In-memory dynamic array with atomic binary database (`CTMs`) and optional CSV interchange format.

### Project 15: Dictionary Application
- **Dynamic Hash Table / Sorted Dictionary**: Case-insensitive vocabulary store featuring binary search for exact matches and linear scans for prefix/substring queries.
- **Curated Vocabulary Seed**: Automatic loading of 32 starter terms from `data/seed_words.txt` on cold start with self-healing `.bak` recovery.
- **Word of the Day**: Deterministic day-of-year hash generating daily featured vocabulary words.

### Project 16: Tic-Tac-Toe
- **Minimax Adversarial AI**: Recursive minimax algorithm with depth scoring guaranteeing optimal play; unbeatable by human opponents.
- **Game Modes**: Two-Player Pass & Play, Player vs Easy AI (random move generator), and Player vs Minimax AI.
- **Scoreboard Persistence**: Multi-round match tracker recording wins, losses, and draws across game sessions (`TTTs`).


---

## 5. Testing & Quality Assurance Framework

### 5.1 Programmatic Unit Testing
Each project features a dedicated C test suite linking against project modules:
- Compiles with AddressSanitizer (`-fsanitize=address`) and UndefinedBehaviorSanitizer (`-fsanitize=undefined`).
- Asserts state mutations, boundary limits, negative inputs, and round-trip persistence.
- Verifies dynamic reallocation and deallocation via clean sanitizers report.

### 5.2 Automated Headless Integration Testing (`test_menu.sh`)
- Written in portable Bash with `set -euo pipefail`.
- Generates an isolated temporary directory using `mktemp -d`.
- Overrides `DATA_DIR` to prevent polluting production or working files.
- Simulates realistic multi-step user sessions using piped heredoc streams.
- Verifies that data persists across cold restarts by launching the binary a second time against the generated data file.
- Automatically destroys temporary sandboxes upon test exit via `trap 'rm -rf "$TMPDIR"' EXIT`.

---

## 6. Execution & Build Pipeline

```bash
# Compile entire portfolio across all 16 projects
make all

# Run complete test battery (unit tests with ASan/UBSan + integration scripts)
make test

# Clean all build binaries, object files, and test outputs
make clean
```

For unified orchestration, the repository includes `run_all.sh` providing interactive execution, individual testing, automated demos, and colorized test telemetry.
