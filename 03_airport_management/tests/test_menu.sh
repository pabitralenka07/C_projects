#!/usr/bin/env bash
set -euo pipefail

PROG="${1:-build/airport}"
if [ ! -x "$PROG" ]; then
    echo "Error: Binary '$PROG' not found or not executable."
    exit 1
fi

TMPDIR=$(mktemp -d /tmp/air_test_XXXXXX)
trap 'rm -rf "$TMPDIR"' EXIT

export DATA_DIR="$TMPDIR"

echo "=== Running Airport Menu Integration Test ==="

# Stdin commands:
# 1 (View Board) -> Enter
# 3 (Book Ticket) -> 1 (Flight ID) -> Claude Shannon -> US-1916 -> Enter
# 2 (Check In) -> 1 (Passenger ID) -> Enter
# 4 (Update Status) -> 1 (Flight ID) -> 3 (Boarding) -> Enter
# 6 (Manifest) -> 1 -> Enter
# 9 (Exit)
"$PROG" << 'EOF' > "$TMPDIR/session1.log" 2>&1
1

3
1
Claude Shannon
US-1916

2
1

4
1
3

6
1

9
EOF

grep -q "Ticket booked successfully! Passenger ID: #1" "$TMPDIR/session1.log" || {
    echo "FAIL: Passenger ticket was not booked."
    cat "$TMPDIR/session1.log"
    exit 1
}

grep -q "Check-in successful! Seat \[01\] confirmed" "$TMPDIR/session1.log" || {
    echo "FAIL: Check-in was not confirmed."
    cat "$TMPDIR/session1.log"
    exit 1
}

grep -q "status updated to 'Boarding'" "$TMPDIR/session1.log" || {
    echo "FAIL: Flight status was not updated."
    cat "$TMPDIR/session1.log"
    exit 1
}

# Session 2: Reload and verify persistence
"$PROG" << 'EOF' > "$TMPDIR/session2.log" 2>&1
6
1

9
EOF

grep -q "Claude Shannon" "$TMPDIR/session2.log" || {
    echo "FAIL: Checked-in passenger not found in reloaded flight manifest."
    cat "$TMPDIR/session2.log"
    exit 1
}

echo "  [PASS] Airport menu integration test passed!"
