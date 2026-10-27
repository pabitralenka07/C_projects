#!/usr/bin/env bash
# Integration test for Currency Converter
set -euo pipefail

PROG="${1:-build/converter}"
if [ ! -x "$PROG" ]; then
    echo "Error: Binary '$PROG' not found or not executable."
    exit 1
fi

TMPDIR=$(mktemp -d /tmp/curr_menu_test_XXXXXX)
trap 'rm -rf "$TMPDIR"' EXIT

export DATA_DIR="$TMPDIR"

echo "=== Running Currency Converter Menu Integration Test ==="

# Test 1: Immediate clean exit on EOF
echo "--- Testing clean exit on EOF (< /dev/null) ---"
if "$PROG" < /dev/null > "$TMPDIR/eof_test.log" 2>&1; then
    echo "  [PASS] Program exited cleanly on EOF (< /dev/null)"
else
    echo "  [FAIL] Program failed to exit cleanly on EOF!"
    exit 1
fi

# Test 2: Convert currency, view table, update rate, and exit
echo "--- Testing interactive conversion and rate update ---"
"$PROG" << 'EOF' > "$TMPDIR/session1.log" 2>&1
1
USD
EUR
100.00

2

3
EUR
0.935000

4
EOF

grep -q "Conversion Result" "$TMPDIR/session1.log" || (echo "Conversion failed"; exit 1)
grep -q "Exchange Rates" "$TMPDIR/session1.log" || (echo "View rates failed"; exit 1)
grep -q "Rate for EUR updated" "$TMPDIR/session1.log" || (echo "Update rate failed"; exit 1)
grep -q "Rates are static sample data" "$TMPDIR/session1.log" || (echo "Disclaimer missing"; exit 1)
echo "  [PASS] Session: Converted currency, viewed rates table, updated rate successfully."

echo "=== All Currency Converter Integration Tests Passed Successfully! ==="
