# Project 12: Currency Converter

A precision financial conversion utility operating on 8+ world currencies with strict input validation, integer minor-unit arithmetic, and persistent file-based exchange rate management.

> **IMPORTANT DISCLAIMER**:  
> **All currency exchange rates in this application and in `data/rates.txt` are static sample data provided solely for educational and demonstration purposes. No live API or internet connections are used.**

---

## Supported Currencies (8+)

| Code | Currency Name | Decimals | Sample Rate to USD (Base: 1.0) |
|:---:|---|:---:|:---:|
| **USD** | US Dollar | 2 | 1.000000 |
| **EUR** | Euro | 2 | 0.920000 |
| **GBP** | British Pound | 2 | 0.790000 |
| **JPY** | Japanese Yen | 0 | 152.350000 |
| **CAD** | Canadian Dollar | 2 | 1.380000 |
| **AUD** | Australian Dollar | 2 | 1.510000 |
| **CHF** | Swiss Franc | 2 | 0.860000 |
| **INR** | Indian Rupee | 2 | 84.050000 |

---

## Features

- **As-Of Date Tracking**: Every conversion and rates listing explicitly prints the "Rates as of" date.
- **Defensive Financial Arithmetic**:
  - Rejects zero, negative amounts, `NaN`, `+Inf`, and `-Inf` via `!(amount >= min && amount <= max)`.
  - Computes exact integer minor units (e.g. cents, paise, yen) using `llround()` to eliminate floating-point drift.
- **Rates Table Management**: View exchange rates and update rates interactively, saving modifications atomically to `data/rates.txt`.

---

## Build & Run

```bash
# Compile
make

# Run unit tests (with ASan + UBSan) and integration tests
make test

# Launch interactively
./build/converter
```
