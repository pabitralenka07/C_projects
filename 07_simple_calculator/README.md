# Project 07: Simple Calculator

A versatile command-line calculator implemented in standard C (C11). Features a recursive-descent expression evaluator supporting operator precedence, grouping parentheses, power operations, basic memory storage, and session history.

---

## Features

- **Expression Evaluation**:
  - Full recursive-descent parser supporting `+`, `-`, `*`, `/`, `%`, and `^` (exponentiation).
  - Proper mathematical precedence and right-associative exponentiation (`2 ^ 3 ^ 2 = 512`).
  - Arbitrary nested parentheses: `((1 + 2) * (3 + 4))`.
  - Unary operators (e.g. `-5 * +3`).
  - Safe error reporting: division by zero, modulo by zero, syntax errors, and domain errors.
- **Standard Arithmetic Mode**:
  - Step-by-step operand and operator prompting with strict input validation.
- **Memory Operations**:
  - Store current or previous result in memory, recall memory value, or clear memory.
- **Session History**:
  - Automatic circular history log storing the last 20 evaluations.

---

## Build and Run

```bash
# Build the binary
make

# Run the calculator
./build/calculator

# Run unit tests and integration tests with ASan + UBSan
make test

# Clean build artifacts
make clean
```

---

## Suggested Enhancements

1. **Infix-to-Postfix (RPN) Mode**: Interactive RPN stack calculator mode for advanced users.
2. **Variable Storage**: Support user-defined named variables (e.g. `x = 10`, `y = 2 * x`).
3. **Scientific Functions**: Trigonometric (`sin`, `cos`, `tan`), logarithmic (`log`, `ln`), and square root (`sqrt`).
