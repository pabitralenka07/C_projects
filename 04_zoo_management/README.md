# Project 04: Zoo Management System

A comprehensive wildlife conservation and zoological garden operations platform implemented in standard C (C11). Features animal registries, dietary feeding logs, habitat occupancy tracking, staff zookeeper rosters, gate admissions ticketing, and crash-safe binary persistence.

---

## Features

- **Animal Registry & Feeding Schedules**:
  - Track animal taxonomy, species, age, habitat assignments, and dietary feeding schedules.
  - Inter-habitat transfer workflows enforcing enclosure capacity ceilings.
- **Enclosure & Habitat Tracking**:
  - Maintain specialized exhibits (e.g. Aviary, Open Savannah, Aquatic, Reptile Terrarium).
  - Track real-time occupancies to prevent overcrowded exhibits.
- **Staff & Keeper Rosters**:
  - Manage zoologists, keepers, veterinarians, and curators mapped to habitats.
- **Gate Ticketing & Admissions Accounting**:
  - Tiered ticketing (Adult, Child, Senior) with running gate revenue reporting.
- **Crash-Safe Persistence**:
  - Dedicated binary storage (`ZOOa`, `ZOOe`, `ZOOs`, `ZOOt` magic headers) with atomic writes (`.tmp` $\to$ `fflush` $\to$ `fsync` $\to$ `.bak` $\to$ atomic `rename`).

---

## Build and Run

```bash
# Build the binary
make

# Run the zoo management system
./build/zoo

# Run unit tests and integration tests with ASan + UBSan
make test

# Clean build artifacts
make clean
```

---

## Suggested Enhancements

1. **Veterinary Health & Vaccination Logs**: Maintain historical medical charts and vaccination reminders per animal.
2. **Visitor Thermal Heatmaps**: Track high-traffic exhibit zones during peak visitor hours.
3. **Feed Inventory Stock Management**: Forecast required food stock based on animal feeding schedules and weights.
