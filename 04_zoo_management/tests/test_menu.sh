#!/usr/bin/env bash
set -euo pipefail

PROG="${1:-build/zoo}"
if [ ! -x "$PROG" ]; then
    echo "Error: Binary '$PROG' not found or not executable."
    exit 1
fi

TMPDIR=$(mktemp -d /tmp/zoo_test_XXXXXX)
trap 'rm -rf "$TMPDIR"' EXIT

export DATA_DIR="$TMPDIR"

echo "=== Running Zoo Menu Integration Test ==="

# Stdin commands:
# 1 (Animals) -> 1 (Add Animal) -> Leo -> Panthera pardus -> 4 -> 1 (Enc ID) -> 10:00 Feed -> Enter -> 5 (Back)
# 4 (Ticketing) -> 1 (Sell) -> 1 (Adult) -> 4 -> Enter -> 2 (Report) -> Enter -> 3 (Back)
# 1 (Animals) -> 2 (List) -> Enter -> 5 (Back)
# 5 (Exit)
"$PROG" << 'EOF' > "$TMPDIR/session1.log" 2>&1
1
1
Leo
Panthera pardus
4
1
10:00 Feed

5
4
1
1
4

2

3
1
2

5
5
EOF

grep -q "Animal registered successfully! (ID:" "$TMPDIR/session1.log" || {
    echo "FAIL: Animal was not registered."
    cat "$TMPDIR/session1.log"
    exit 1
}

grep -q "Sold 4 ticket(s) for \$80.00" "$TMPDIR/session1.log" || {
    echo "FAIL: Ticket sale was not completed."
    cat "$TMPDIR/session1.log"
    exit 1
}

# Session 2: Reload and verify persistence
"$PROG" << 'EOF' > "$TMPDIR/session2.log" 2>&1
1
2

5
5
EOF

grep -q "Leo" "$TMPDIR/session2.log" || {
    echo "FAIL: Registered animal not reloaded."
    cat "$TMPDIR/session2.log"
    exit 1
}

echo "  [PASS] Zoo management menu integration test passed!"
