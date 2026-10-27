#!/usr/bin/env bash
set -euo pipefail

PROG="${1:-build/factorial}"
if [ ! -x "$PROG" ]; then
    echo "Error: Binary '$PROG' not found or not executable."
    exit 1
fi

TMPDIR=$(mktemp -d /tmp/fact_test_XXXXXX)
trap 'rm -rf "$TMPDIR"' EXIT

echo "=== Running Factorial Menu Integration Test ==="

# Stdin commands:
# 1 (Standard) -> 10 -> Enter
# 2 (Big-Int) -> 50 -> Enter
# 3 (Permutations) -> 10 -> 2 -> Enter
# 4 (Combinations) -> 52 -> 5 -> Enter
# 6 (Exit)
"$PROG" << 'EOF' > "$TMPDIR/session.log" 2>&1
1
10

2
50

3
10
2

4
52
5

6
EOF

grep -q "10! = 3628800" "$TMPDIR/session.log" || {
    echo "FAIL: Expected 10! = 3628800."
    cat "$TMPDIR/session.log"
    exit 1
}

grep -q "Total digits: 65" "$TMPDIR/session.log" || {
    echo "FAIL: Expected 50! to have 65 digits."
    cat "$TMPDIR/session.log"
    exit 1
}

grep -q "P(10, 2) = 90" "$TMPDIR/session.log" || {
    echo "FAIL: Expected P(10, 2) = 90."
    cat "$TMPDIR/session.log"
    exit 1
}

grep -q "C(52, 5) = 2598960" "$TMPDIR/session.log" || {
    echo "FAIL: Expected C(52, 5) = 2598960."
    cat "$TMPDIR/session.log"
    exit 1
}

echo "  [PASS] Factorial menu integration test passed!"
