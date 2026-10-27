# Project 13: ATM System Simulation

A hardware-oriented simulation of an Automated Teller Machine (ATM) terminal session, featuring card validation, salted PIN authentication, note denomination dispensing, daily limits, mini statement audit logs, and automatic card lockout.

> **EDUCATIONAL SIMULATION NOTICE**:  
> **This application is an educational simulation of an ATM terminal workflow. It demonstrates salted hashing and constant-time string comparisons without external libraries, but is not intended for real-world production cryptographic banking authentication.**

---

## Architecture & Distinct ATM Features

Unlike branch banking systems (such as Project 06), this system explicitly models the physical constraints of an ATM terminal:
- **Card Insertion & Session**: User inserts a 16-digit card number and enters a 4-digit PIN.
- **Physical Cash Dispensing**: Withdrawals are strictly dispensed in **$20 note denominations**.
- **Daily Withdrawal Caps**: Enforces a strict daily limit of **$1,000.00** per card.
- **ATM Cash Tray Reserve**: Verifies the ATM terminal itself has sufficient physical cash reserves before dispensing.
- **Three-Strike Security Lockout**: Tracking 3 consecutive incorrect PIN entries triggers immediate card lockout (`is_locked = 1`).
- **Mini Statement**: Displays the last 5 transactions in reverse chronological order.
- **PIN Change**: Validates the existing PIN before computing a new random salt and hash.

---

## Default Sample Cards (Auto-Seeded on First Run)

| Card Number | Account Holder | Default PIN | Initial Balance |
|---|---|:---:|:---:|
| `1111222233334444` | Alice Smith | `1234` | $1,500.00 |
| `5555666677778888` | Bob Jones | `4321` | $800.00 |

---

## Build & Run

```bash
# Compile
make

# Run unit tests (with ASan + UBSan) and integration tests
make test

# Launch interactively
./build/atm
```
