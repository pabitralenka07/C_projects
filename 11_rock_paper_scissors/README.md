# Project 11: Rock Paper Scissors

A production-grade implementation of Rock Paper Scissors featuring Best-of-N matches, streak calculation, persistent lifetime metrics, and an injectable pseudo-random number generator for deterministic automated verification.

---

## Features

- **Match Modes**: Quick Match (1 round), Best-of-3, Best-of-5, or customizable odd Best-of-N.
- **Session & Lifetime Metrics**: Tracks matches played, rounds won, lost, and drawn, percentage win rates, current win streak, and all-time best streak.
- **Deterministic RNG Injection**: The computer move generator is abstracted behind an injectable callback signature `RpsMove (*RpsRngFunc)(void *ctx)`, enabling deterministic unit testing.
- **Crash-Resilient Persistence**: Saves statistics to `data/rps_stats.dat` using `safe_atomic_save` (magic bytes `RPSs`).

---

## Build & Run

```bash
# Build
make

# Run unit tests (with ASan + UBSan) and integration tests
make test

# Launch interactively
./build/rps
```
