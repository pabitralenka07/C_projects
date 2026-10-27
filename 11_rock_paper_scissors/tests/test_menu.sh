#!/usr/bin/env bash
# Integration test for Rock Paper Scissors
set -euo pipefail

PROG="${1:-build/rps}"
if [ ! -x "$PROG" ]; then
    echo "Error: Binary '$PROG' not found or not executable."
    exit 1
fi

TMPDIR=$(mktemp -d /tmp/rps_menu_test_XXXXXX)
trap 'rm -rf "$TMPDIR"' EXIT

export DATA_DIR="$TMPDIR"
export RPS_SEED=42

echo "=== Running Rock Paper Scissors Menu Integration Test ==="

# Test 1: Immediate clean exit on EOF
echo "--- Testing clean exit on EOF (< /dev/null) ---"
if "$PROG" < /dev/null > "$TMPDIR/eof_test.log" 2>&1; then
    echo "  [PASS] Program exited cleanly on EOF (< /dev/null)"
else
    echo "  [FAIL] Program failed to exit cleanly on EOF!"
    exit 1
fi

# Test 2: Play round, view stats, reset, and exit
echo "--- Testing match play and statistics ---"
"$PROG" << 'EOF' > "$TMPDIR/session1.log" 2>&1
1
p

5

6
y

7
EOF

grep -q "Starting Best-of-1 Match" "$TMPDIR/session1.log" || (echo "Match start failed"; exit 1)
grep -q "RPS Lifetime Statistics" "$TMPDIR/session1.log" || (echo "View stats failed"; exit 1)
grep -q "Statistics reset to zero" "$TMPDIR/session1.log" || (echo "Reset stats failed"; exit 1)
echo "  [PASS] Session: Played round, checked stats, reset stats successfully."

echo "=== All Rock Paper Scissors Integration Tests Passed Successfully! ==="
