#!/usr/bin/env bash
# Integration test for Contact Management System
set -euo pipefail

PROG="${1:-build/contact_manager}"
if [ ! -x "$PROG" ]; then
    echo "Error: Binary '$PROG' not found or not executable."
    exit 1
fi

TMPDIR=$(mktemp -d /tmp/contact_menu_test_XXXXXX)
trap 'rm -rf "$TMPDIR"' EXIT

export DATA_DIR="$TMPDIR"

echo "=== Running Contact Management Menu Integration Test ==="

# Test 1: Immediate clean exit on EOF (test with < /dev/null)
echo "--- Testing clean exit on EOF (< /dev/null) ---"
if "$PROG" < /dev/null > "$TMPDIR/eof_test.log" 2>&1; then
    echo "  [PASS] Program exited cleanly on EOF (< /dev/null)"
else
    echo "  [FAIL] Program failed to exit cleanly on EOF!"
    exit 1
fi

# Test 2: Multi-step interactive session
# Commands:
# 1 (Add Contact) -> Alan Turing -> 555-123-4567 -> alan@bletchley.gov -> Bletchley Park -> Cipher -> Enter
# 1 (Add Contact) -> Joan Clarke -> 555-987-6543 -> joan@bletchley.gov -> Hut 8 -> Cryptanalysis -> Enter
# 4 (Search) -> bletchley -> Enter
# 5 (List) -> 1 (Sort Name ASC) -> Enter
# 6 (Export CSV) -> $TMPDIR/export.csv -> Enter
# 2 (Edit Contact) -> 1 -> Dr. Alan M. Turing -> [Enter] -> [Enter] -> [Enter] -> [Enter] -> Enter
# 3 (Delete Contact) -> 2 -> n (Cancel deletion) -> Enter
# 3 (Delete Contact) -> 2 -> y (Confirm deletion) -> Enter
# 8 (Save & Exit)
echo "--- Testing interactive menu operations ---"
"$PROG" << 'EOF' > "$TMPDIR/session1.log" 2>&1
1
Alan Turing
555-123-4567
alan@bletchley.gov
Bletchley Park
Cipher

1
Joan Clarke
555-987-6543
joan@bletchley.gov
Hut 8
Cryptanalysis

4
bletchley

5
1

6
$TMPDIR/export.csv

2
1
Dr. Alan M. Turing





3
2
n

3
2
y

8
EOF

# Verify outcomes in session log
grep -q "Contact added successfully with ID #1" "$TMPDIR/session1.log" || (echo "Failed to add contact 1"; exit 1)
grep -q "Contact added successfully with ID #2" "$TMPDIR/session1.log" || (echo "Failed to add contact 2"; exit 1)
grep -q "Alan Turing" "$TMPDIR/session1.log" || (echo "Search failed"; exit 1)
grep -q "Contact #1 updated successfully" "$TMPDIR/session1.log" || (echo "Edit failed"; exit 1)
grep -q "Deletion canceled" "$TMPDIR/session1.log" || (echo "Cancel deletion failed"; exit 1)
grep -q "Contact #2 has been deleted" "$TMPDIR/session1.log" || (echo "Confirm deletion failed"; exit 1)
echo "  [PASS] Session 1: Add, search, edit, cancel delete, confirm delete passed."

# Test 3: Cold reload and verify persistence
echo "--- Testing persistence reload ---"
"$PROG" << 'EOF' > "$TMPDIR/session2.log" 2>&1
4
Alan

8
EOF

grep -q "Dr. Alan M. Turing" "$TMPDIR/session2.log" || (echo "Persistence reload failed"; exit 1)
echo "  [PASS] Session 2: Cold restart verified persisted contact data."

# Test 4: Delete prompt with EOF (EOF must mean "no")
echo "--- Testing confirmation prompt on EOF ---"
"$PROG" << 'EOF' > "$TMPDIR/session3.log" 2>&1
3
1
EOF

grep -q "Deletion canceled" "$TMPDIR/session3.log" || (echo "EOF deletion did not cancel"; exit 1)
echo "  [PASS] Session 3: Deletion prompt canceled safely when stream reached EOF."

echo "=== All Contact Management Integration Tests Passed Successfully! ==="
