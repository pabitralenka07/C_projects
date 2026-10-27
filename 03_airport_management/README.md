# Project 03: Airport Management System

An airport flight operations and terminal passenger management system implemented in standard C (C11). Features live departure/arrival status boards, gate reassignments, passenger check-in workflows, boarding pass generation, passenger manifest auditing, and crash-safe binary persistence.

---

## Features

- **Live Flight Operations Board**:
  - Track flight statuses: *On Time*, *Delayed*, *Boarding*, *Departed*, and *Cancelled*.
  - Update gate assignments dynamically across terminal concourses.
- **Ticketing & Passenger Check-In**:
  - Book flights with passenger demographic and passport verification.
  - Check-in workflow enforcing aircraft seat capacity constraints.
  - Printable ASCII Boarding Pass receipt generator.
- **Flight Manifests**:
  - Comprehensive crew manifest detailing boarded vs unconfirmed passengers by seat number.
- **Crash-Safe Persistence**:
  - Binary storage (`AIRf` and `AIRp` magic headers) with atomic writes (`.tmp` $\to$ `fflush` $\to$ `fsync` $\to$ `.bak` $\to$ atomic `rename`).

---

## Build and Run

```bash
# Build the binary
make

# Run the airport management system
./build/airport

# Run unit tests and integration tests with ASan + UBSan
make test

# Clean build artifacts
make clean
```

---

## Suggested Enhancements

1. **Luggage & Baggage Tag Tracking**: Track checked bags and carousel assignments.
2. **Weather Delay Simulator**: Procedurally generate airport weather patterns impacting departure schedules.
3. **Multi-Terminal Concourses**: Track passenger walking distance and gate connections across multiple airport terminals.
