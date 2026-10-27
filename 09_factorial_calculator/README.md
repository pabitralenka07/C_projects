# Project 09: Factorial & Combinatorics Calculator

A high-performance computational tool for factorials and combinatorics in standard C (C11). Supports standard 64-bit unsigned integers ($n \le 20$), arbitrary-precision big-integer multiplication ($n \le 1000$), permutations $P(n, r)$, combinations $C(n, r)$, and Stirling's asymptotic approximation.

---

## Features

- **64-bit Unsigned Fast Mode**:
  - Computes exact factorials up to $20! = 2,432,902,008,176,640,000$.
  - Explicit overflow detection preventing silent wrapping on $n \ge 21$.
- **Arbitrary Precision Big-Integer Engine**:
  - Custom base-10 carry multiplication algorithm supporting exact factorials up to $1000!$.
  - Example: computes all 158 digits of $100!$ with exact trailing zero precision.
- **Permutations and Combinations**:
  - $P(n, r)$ and $C(n, r)$ utilizing multiplicative cancellation to compute combinations without intermediate factorial overflows.
- **Stirling's Asymptotic Approximation**:
  - Calculates $n! \approx \sqrt{2\pi n} (n / e)^n$ and computes real-time relative error percentage against exact values.

---

## Build and Run

```bash
# Build the binary
make

# Run the calculator
./build/factorial

# Run unit tests and integration tests with ASan + UBSan
make test

# Clean build artifacts
make clean
```

---

## Suggested Enhancements

1. **Sub-Factorial (Derangements)**: Compute $!n = \lfloor \frac{n!}{e} + \frac{1}{2} \rfloor$.
2. **Prime Factorization of $n!$**: Use Legendre's formula to decompose large factorials into prime exponents without multiplication.
3. **Multithreaded Multiplication**: Parallelize divide-and-conquer binary splitting for massive factorials ($n > 10,000$).
