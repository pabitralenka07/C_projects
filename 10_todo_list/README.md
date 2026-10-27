# Project 10: To-Do List Manager

A persistent, interactive command-line To-Do list manager built in standard C (C11). Features task prioritization, categories, status filtering, case-insensitive substring searching, and atomic crash-safe binary persistence.

---

## Features

- **Task Management**:
  - Add tasks with custom titles, categories (tags), and priority ratings (Low, Medium, High).
  - Quick toggle between pending and completed states.
  - In-place task editing and soft deletion.
- **Categorization & Filtering**:
  - Filter tasks dynamically by completion status (Pending vs. Done) or priority levels.
  - Substring search across both task titles and categories.
- **Crash-Safe Persistence**:
  - Binary storage (`TDOi` magic header) with atomic writes (`.tmp` $\to$ `fflush` $\to$ `fsync` $\to$ `.bak` $\to$ atomic `rename`).
  - Automatic corrupt file detection and backup recovery.

---

## Build and Run

```bash
# Build the binary
make

# Run the task manager
./build/todo

# Run unit tests and integration tests with ASan + UBSan
make test

# Clean build artifacts
make clean
```

---

## Suggested Enhancements

1. **Due Dates & Overdue Highlighting**: Add timestamp deadlines and visual alarms for approaching tasks.
2. **Export to Markdown / HTML**: Generate clean progress reports formatted as markdown checklists.
3. **Task Dependency Graph**: Prevent completing tasks until prerequisite child tasks are resolved.
