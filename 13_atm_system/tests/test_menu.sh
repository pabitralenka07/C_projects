#!/usr/bin/env bash
# Integration test for ATM Simulation System
set -euo pipefail

PROG="${1:-build/atm}"
if [ ! -x "$PROG" ]; then
    echo "Error: Binary '$PROG' not found or not executable."
    exit 1
fi

TMPDIR=$(mktemp -d /tmp/atm_menu_test_XXXXXX)
trap 'rm -rf "$TMPDIR"' EXIT

export DATA_DIR="$TMPDIR"

echo "=== Running ATM System Menu Integration Test ==="

# Test 1: Immediate clean exit on EOF
echo "--- Testing clean exit on EOF (< /dev/null) ---"
if "$PROG" < /dev/null > "$TMPDIR/eof_test.log" 2>&1; then
    echo "  [PASS] Program exited cleanly on EOF (< /dev/null)"
else
    echo "  [FAIL] Program failed to exit cleanly on EOF!"
    exit 1
fi

# Test 2: Standard cardholder workflow: login, check balance, withdraw, deposit, mini statement, eject
echo "--- Testing interactive cardholder session ---"
"$PROG" << 'EOF' > "$TMPDIR/session1.log" 2>&1
1
1111222233334444
1234

1

2
40.00

3
100.00

4

6

4
EOF

grep -q "Authentication successful. Welcome, Alice Smith" "$TMPDIR/session1.log" || (echo "Login failed"; exit 1)
grep -q "Account Balance" "$TMPDIR/session1.log" || (echo "Check balance failed"; exit 1)
grep -q "Please take your 2 x \$20 bills" "$TMPDIR/session1.log" || (echo "Withdraw failed"; exit 1)
grep -q "Deposited \$100.00 successfully" "$TMPDIR/session1.log" || (echo "Deposit failed"; exit 1)
grep -q "ATM MINI STATEMENT" "$TMPDIR/session1.log" || (echo "Mini statement failed"; exit 1)
grep -q "CARD RETURNED" "$TMPDIR/session1.log" || (echo "Card return failed"; exit 1)
echo "  [PASS] Session: Login, balance, withdraw, deposit, mini-statement, card return succeeded."

# Test 3: Card lockout after 3 consecutive failed PIN attempts
echo "--- Testing 3-strike card lockout ---"
"$PROG" << 'EOF' > "$TMPDIR/session2.log" 2>&1
1
1111222233334444
0001

1
1111222233334444
0002

1
1111222233334444
0003

4
EOF

grep -q "2 attempt(s) remaining" "$TMPDIR/session2.log" || (echo "Lockout attempt 1 failed"; exit 1)
grep -q "1 attempt(s) remaining" "$TMPDIR/session2.log" || (echo "Lockout attempt 2 failed"; exit 1)
grep -q "SECURITY LOCKOUT" "$TMPDIR/session2.log" || (echo "Card lockout failed"; exit 1)
echo "  [PASS] Card lockout triggered correctly after 3 failed PIN entries."

echo "=== All ATM System Integration Tests Passed Successfully! ==="
