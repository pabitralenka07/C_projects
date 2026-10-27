#!/usr/bin/env bash
set -euo pipefail

PROG="${1:-build/hospital}"
if [ ! -x "$PROG" ]; then
    echo "Error: Binary '$PROG' not found or not executable."
    exit 1
fi

TMPDIR=$(mktemp -d /tmp/hosp_test_XXXXXX)
trap 'rm -rf "$TMPDIR"' EXIT

export DATA_DIR="$TMPDIR"

echo "=== Running Hospital Menu Integration Test ==="

# Stdin commands:
# 2 (Doctor Menu) -> 1 (Add Doctor) -> Dr. Jonas Salk -> Virology -> 555-1953 -> Enter -> 5 (Back)
# 1 (Patient Menu) -> 1 (Admit) -> Albert Sabin -> 45 -> M -> 555-1960 -> Polio Research -> 1 (Doc ID) -> Enter
# 1 (Patient Menu) -> 2 (List Active) -> Enter -> 5 (Discharge) -> 1 (Pat ID) -> 250.00 -> Enter -> 6 (Back)
# 4 (Billing Menu) -> 1 (List Bills) -> Enter -> 2 (Pay Bill) -> 1 -> Enter -> 4 (Back)
# 5 (Unified Search) -> "Sabin" -> Enter
# 6 (Exit)
"$PROG" << 'EOF' > "$TMPDIR/session1.log" 2>&1
2
1
Dr. Jonas Salk
Virology
555-1953

5
1
1
Albert Sabin
45
M
555-1960
Polio Research
1

2

5
1
250.00

6
4
1

2
1

4
5
Sabin

6
EOF

grep -q "Doctor added successfully! (ID: 1)" "$TMPDIR/session1.log" || {
    echo "FAIL: Doctor was not added."
    cat "$TMPDIR/session1.log"
    exit 1
}

grep -q "Patient admitted successfully! (ID: 1)" "$TMPDIR/session1.log" || {
    echo "FAIL: Patient was not admitted."
    cat "$TMPDIR/session1.log"
    exit 1
}

grep -q "discharged! Generated Bill #1" "$TMPDIR/session1.log" || {
    echo "FAIL: Patient was not discharged with bill."
    cat "$TMPDIR/session1.log"
    exit 1
}

grep -q "Payment received for Bill #1" "$TMPDIR/session1.log" || {
    echo "FAIL: Bill payment failed."
    cat "$TMPDIR/session1.log"
    exit 1
}

echo "  [PASS] Session 1: Doctor added, patient admitted, discharged with bill, and paid."

# Session 2: Reload and verify persistence
"$PROG" << 'EOF' > "$TMPDIR/session2.log" 2>&1
1
3

6
6
EOF

grep -q "Albert Sabin" "$TMPDIR/session2.log" || {
    echo "FAIL: Persisted patient not reloaded."
    cat "$TMPDIR/session2.log"
    exit 1
}

echo "  [PASS] Session 2: Verified persistence reload."
echo "=== Hospital Management Integration Tests Passed! ==="
