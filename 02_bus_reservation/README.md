# Project 02: Bus Reservation System

An interactive bus booking and seat allocation system implemented in standard C (C11). Features ASCII 2+2 seat map rendering, seat selection, booking cancellations, boarding pass receipt generation, route searching, and crash-safe binary persistence.

---

## Features

- **Interactive Seat Map**:
  - Visual 2+2 seating layout rendering showing occupied (`[XX]`) versus available (`[01]`) seats.
  - Prevents double-booking or booking out-of-range seats.
- **Ticketing & Cancellation**:
  - Issue tickets with unique booking IDs, passenger details, assigned seat numbers, and timestamps.
  - Cancel bookings to immediately restore seat availability.
  - Printable ASCII boarding pass / receipt generator.
- **Route Searching**:
  - Multi-criteria route search by origin, destination, or time of departure.
- **Crash-Safe Persistence**:
  - Binary storage (`BUSb` and `BUSk` magic headers) with atomic writes (`.tmp` $\to$ `fflush` $\to$ `fsync` $\to$ `.bak` $\to$ atomic `rename`).

---

## Build and Run

```bash
# Build the binary
make

# Run the bus reservation system
./build/bus_reservation

# Run unit tests and integration tests with ASan + UBSan
make test

# Clean build artifacts
make clean
```

---

## Suggested Enhancements

1. **Dynamic Pricing**: Adjust seat fares based on proximity to departure and remaining seat inventory.
2. **Intermediate Bus Stops**: Support boarding and alighting across multi-stop long-distance corridors.
3. **SMS / Email Mock Dispatch**: Simulate sending mobile booking confirmations and QR code boarding tokens.
