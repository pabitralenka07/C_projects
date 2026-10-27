# Project 08: Palindrome Checker

A high-performance palindrome verification tool implemented in standard C (C11). Supports strict string comparison, relaxed sentence verification (ignoring whitespace, punctuation, and letter case), direct numeric integer palindrome testing, and longest palindromic substring discovery.

---

## Features

- **Strict & Relaxed String Modes**:
  - Two-pointer verification algorithm ($O(n)$ time, $O(1)$ auxiliary memory).
  - Handles complex palindromic sentences (e.g., *"A man, a plan, a canal: Panama!"*).
- **Pure Arithmetic Integer Checking**:
  - Reverses digits using arithmetic operators without allocating strings.
  - Properly handles negative numbers and zero.
- **Longest Palindromic Substring**:
  - Expand-around-center algorithm identifying the longest palindromic sequence in arbitrary text.

---

## Build and Run

```bash
# Build the binary
make

# Run the palindrome checker
./build/palindrome

# Run unit tests and integration tests with ASan + UBSan
make test

# Clean build artifacts
make clean
```

---

## Suggested Enhancements

1. **Unicode / UTF-8 Normalization**: Support multilingual accented characters and case folding.
2. **Batch File Processing**: Inspect large text files line-by-line and export all palindromic phrases.
3. **Manacher's Algorithm**: Upgrade longest palindromic substring search to strictly $O(n)$ linear time.
