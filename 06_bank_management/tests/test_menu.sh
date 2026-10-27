#!/usr/bin/env bash
set -euo pipefail

PROG="${1:-build/bank}"
if [ ! -x "$PROG" ]; then
    echo "Error: Binary '$PROG' not found or not executable."
    exit 1
fi

TMPDIR=$(mktemp -d /tmp/bank_test_XXXXXX)
trap 'rm -rf "$TMPDIR"' EXIT

export DATA_DIR="$TMPDIR"

echo "=== Running Bank Menu Integration Test ==="

# Stdin commands:
# 1 (Open Account) -> Margaret Hamilton -> 555-1936 -> 7777 -> 1000.00 -> Enter
# 2 (Deposit) -> 1003 (Third account after sample Alice & Bob) -> 500.00 -> Enter
# 3 (Withdraw) -> 1003 -> 7777 -> 200.00 -> Enter
# 4 (Transfer) -> 1003 -> 7777 -> 1001 (Alice) -> 300.00 -> Enter
# 5 (Statement) -> 1003 -> 7777 -> Enter
# 8 (Exit)
"$PROG" << 'EOF' > "$TMPDIR/session1.log" 2>&1
1
Margaret Hamilton
555-1936
7777
1000.00

2
1003
500.00

3
1003
7777
200.00

4
1003
7777
1001
300.00

5
1003
7777

8
EOF

grep -q "Account opened successfully" "$TMPDIR/session1.log" || {
    echo "FAIL: Account was not opened."
    cat "$TMPDIR/session1.log"
    exit 1
}

grep -q "Deposit successful! New Balance: \$1500.00" "$TMPDIR/session1.log" || {
    echo "FAIL: Deposit was not processed."
    cat "$TMPDIR/session1.log"
    exit 1
}

grep -q "Withdrawal of \$200.00 approved! New Balance: \$1300.00" "$TMPDIR/session1.log" || {
    echo "FAIL: Withdrawal was not processed."
    cat "$TMPDIR/session1.log"
    exit 1
}

grep -q "Transfer successful! Sent \$300.00" "$TMPDIR/session1.log" || {
    echo "FAIL: Wire transfer failed."
    cat "$TMPDIR/session1.log"
    exit 1
}

# Session 2: Reload and verify persistence
"$PROG" << 'EOF' > "$TMPDIR/session2.log" 2>&1
6

8
EOF

grep -q "Margaret Hamilton" "$TMPDIR/session2.log" || {
    echo "FAIL: Account not reloaded from persistent storage."
    cat "$TMPDIR/session2.log"
    exit 1
}

echo "  [PASS] Bank management menu integration test passed!"
