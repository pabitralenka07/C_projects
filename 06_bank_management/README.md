# Project 06: Bank Management System

A secure banking and ledger accounting application implemented in standard C (C11). Features salted PIN security, automated 3-attempt lockout mechanisms, cash deposits, withdrawals with overdraft protection, atomic wire transfers, formatted ledger statements, and crash-safe binary persistence.

---

## Features

- **Salted PIN Security & Lockout**:
  - Educational salted cryptographic hashing storing 64-character digests with 16-character random hex salts.
  - Automatic account freezing upon 3 consecutive incorrect PIN entries.
  - Administrative security override unlocking mechanism.
- **Transactional Operations**:
  - Cash deposits and withdrawals with balance checks preventing overdrafts.
  - Atomic inter-account wire transfers maintaining transaction consistency across both sender and recipient accounts.
- **Audit Ledger Statements**:
  - Generates detailed account statements displaying chronological transaction timestamps, types (`DEPOSIT`, `WITHDRAW`, `TRANSFER`), amounts, and resulting balances.
- **Crash-Safe Persistence**:
  - Dedicated binary storage (`BNKa` and `BNKt` magic headers) with atomic writes (`.tmp` $\to$ `fflush` $\to$ `fsync` $\to$ `.bak` $\to$ atomic `rename`).

---

## Build and Run

```bash
# Build the binary
make

# Run the bank management system
./build/bank

# Run unit tests and integration tests with ASan + UBSan
make test

# Clean build artifacts
make clean
```

---

## Suggested Enhancements

1. **Compound Interest Accrual**: Daily compound interest calculations for savings accounts.
2. **Multi-Currency Support**: Real-time currency conversions with dynamic exchange rates.
3. **Daily Withdrawal Velocity Limits**: Maximum daily ATM transaction caps with velocity monitoring.
