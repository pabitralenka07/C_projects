#!/usr/bin/env bash
set -euo pipefail

PROG="${1:-build/calculator}"
if [ ! -x "$PROG" ]; then
    echo "Error: Binary '$PROG' not found or not executable."
    exit 1
fi

TMPDIR=$(mktemp -d /tmp/calc_test_XXXXXX)
trap 'rm -rf "$TMPDIR"' EXIT

echo "=== Running Calculator Menu Integration Test ==="

# Stdin commands:
# 1 (Evaluate) -> (10 + 20) * 3 -> n (don't store) -> Enter
# 2 (Standard Op) -> 50 -> / -> 2 -> n -> Enter
# 4 (View History) -> Enter
# 5 (Exit)
"$PROG" << 'EOF' > "$TMPDIR/session.log" 2>&1
1
(10 + 20) * 3
n

2
50
/
2
n

4

5
EOF

grep -q "= 90" "$TMPDIR/session.log" || {
    echo "FAIL: Expected result 90 from expression evaluation."
    cat "$TMPDIR/session.log"
    exit 1
}

grep -q "50 / 2 = 25" "$TMPDIR/session.log" || {
    echo "FAIL: Expected 50 / 2 = 25 from standard op."
    cat "$TMPDIR/session.log"
    exit 1
}

grep -q "Calculation History" "$TMPDIR/session.log" || {
    echo "FAIL: History was not displayed."
    cat "$TMPDIR/session.log"
    exit 1
}

echo "  [PASS] Calculator interactive menu integration test passed!"
