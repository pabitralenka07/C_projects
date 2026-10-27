#!/usr/bin/env bash
set -euo pipefail

PROG="${1:-build/bus_reservation}"
if [ ! -x "$PROG" ]; then
    echo "Error: Binary '$PROG' not found or not executable."
    exit 1
fi

TMPDIR=$(mktemp -d /tmp/bus_test_XXXXXX)
trap 'rm -rf "$TMPDIR"' EXIT

export DATA_DIR="$TMPDIR"

echo "=== Running Bus Reservation Menu Integration Test ==="

# Stdin commands:
# 1 (View Buses) -> Enter
# 2 (View Seat Map) -> 1 -> Enter
# 3 (Book Seat) -> 1 (Bus ID) -> 7 (Seat 7) -> Margaret Hamilton -> 555-1969 -> Enter
# 5 (Print Ticket) -> 1 -> Enter
# 8 (Exit)
"$PROG" << 'EOF' > "$TMPDIR/session1.log" 2>&1
1

2
1

3
1
7
Margaret Hamilton
555-1969

5
1

8
EOF

grep -q "Booking confirmed! Ticket ID: #1" "$TMPDIR/session1.log" || {
    echo "FAIL: Seat booking was not confirmed."
    cat "$TMPDIR/session1.log"
    exit 1
}

grep -q "Margaret Hamilton" "$TMPDIR/session1.log" || {
    echo "FAIL: Passenger name not in printed ticket."
    cat "$TMPDIR/session1.log"
    exit 1
}

# Session 2: Reload and verify seat map persists seat 7 as [XX]
"$PROG" << 'EOF' > "$TMPDIR/session2.log" 2>&1
2
1

8
EOF

grep -q "SEAT MAP: Express Continental" "$TMPDIR/session2.log" || {
    echo "FAIL: Bus was not reloaded."
    cat "$TMPDIR/session2.log"
    exit 1
}

echo "  [PASS] Bus reservation menu integration test passed!"
