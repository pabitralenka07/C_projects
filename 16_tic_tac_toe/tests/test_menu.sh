#!/usr/bin/env bash
# Integration test for Tic-Tac-Toe
set -euo pipefail

PROG="${1:-build/tictactoe}"
if [ ! -x "$PROG" ]; then
    echo "Error: Binary '$PROG' not found or not executable."
    exit 1
fi

TMPDIR=$(mktemp -d /tmp/ttt_menu_test_XXXXXX)
trap 'rm -rf "$TMPDIR"' EXIT

echo "=== Running Tic-Tac-Toe Menu Integration Test ==="

# Test 1: Immediate clean exit on EOF
echo "--- Testing clean exit on EOF (< /dev/null) ---"
if "$PROG" < /dev/null > "$TMPDIR/eof_test.log" 2>&1; then
    echo "  [PASS] Program exited cleanly on EOF (< /dev/null)"
else
    echo "  [FAIL] Program failed to exit cleanly on EOF!"
    exit 1
fi

# Test 2: Play deterministic two-player game, view scoreboard, reset scoreboard, exit
echo "--- Testing interactive game session ---"
"$PROG" << 'EOF' > "$TMPDIR/session1.log" 2>&1
3
1
1
2
1
1
2
2
2
1
3

4

5
y

6
EOF

grep -q "New Tic-Tac-Toe Game" "$TMPDIR/session1.log" || (echo "Game start failed"; exit 1)
grep -q "Tic-Tac-Toe Scoreboard" "$TMPDIR/session1.log" || (echo "Scoreboard view failed"; exit 1)
grep -q "Scoreboard reset" "$TMPDIR/session1.log" || (echo "Scoreboard reset failed"; exit 1)
echo "  [PASS] Session: Played game, viewed scoreboard, reset scoreboard succeeded."

echo "=== All Tic-Tac-Toe Integration Tests Passed Successfully! ==="
