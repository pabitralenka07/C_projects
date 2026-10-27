# Project 15: Dictionary Application

An algorithmic dictionary and vocabulary reference application featuring $O(\log N)$ binary search lookup, prefix-based autocomplete search, keyword full-text indexing, Word of the Day recommendations, and crash-resilient atomic binary persistence.

---

## Features

- **Binary Search ($O(\log N)$)**: All vocabulary records are maintained in sorted order. Exact word lookup operates via binary search without scanning the full array.
- **Prefix Autocomplete Search**: Locates the lower bound index via binary search, then scans sequential matches in $O(\log N + K)$ time.
- **Substring & Keyword Search**: Searches definitions and examples for targeted keywords.
- **Word of the Day**: Date-seeded deterministic selection providing a featured daily vocabulary word.
- **Seed Vocabulary Included**: Automatically loads 32 curated computer science and English terms from `data/seed_words.txt` on a fresh start.
- **Atomic Persistence**: Persists vocabulary entries to `data/dictionary.dat` with magic bytes `DICT`.

---

## Build & Run

```bash
# Compile
make

# Run unit tests (with ASan + UBSan) and integration tests
make test

# Launch interactively
./build/dict
```
