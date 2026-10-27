#!/usr/bin/env bash
# Integration test for Library Management System
# Exercises menu flow: add book, add member, issue book, return book, search, and verify persistence.
set -euo pipefail

PROG="${1:-build/library}"
if [ ! -x "$PROG" ]; then
    echo "Error: Binary '$PROG' not found or not executable."
    exit 1
fi

TMPDIR=$(mktemp -d /tmp/lib_menu_test_XXXXXX)
trap 'rm -rf "$TMPDIR"' EXIT

export DATA_DIR="$TMPDIR"

echo "=== Running Menu Integration Test (Session 1: Add & Issue) ==="

# Stdin commands:
# 1 (Book Menu) -> 1 (Add Book) -> Title -> Author -> ISBN -> Year -> Copies -> Enter (press enter) -> 7 (Back)
# 2 (Member Menu) -> 1 (Add Member) -> Name -> Phone -> Email -> Enter -> 7 (Back)
# 3 (Circulation) -> 1 (Issue) -> Book ID (1) -> Member ID (1) -> Enter -> 3 (View Active) -> Enter -> 2 (Return) -> Txn ID (1) -> Enter -> 7 (Back)
# 4 (Search) -> Query ("Clean") -> Enter
# 5 (Exit)

"$PROG" << 'EOF' > "$TMPDIR/session1.log" 2>&1
1
1
Clean Code
Robert C. Martin
978-0132350884
2008
4

7
2
1
Grace Hopper
555-0199
grace@navy.mil

7
3
1
1
1

3

2
1

7
4
Clean

5
EOF

# Assertions on Session 1 log
grep -q "Book added successfully! (ID: 1)" "$TMPDIR/session1.log" || {
    echo "FAIL: Book was not added successfully."
    cat "$TMPDIR/session1.log"
    exit 1
}

grep -q "Member added successfully! (ID: 1)" "$TMPDIR/session1.log" || {
    echo "FAIL: Member was not added successfully."
    cat "$TMPDIR/session1.log"
    exit 1
}

grep -q "Book issued! Transaction ID: 1" "$TMPDIR/session1.log" || {
    echo "FAIL: Book was not issued successfully."
    cat "$TMPDIR/session1.log"
    exit 1
}

grep -q "Book returned successfully!" "$TMPDIR/session1.log" || {
    echo "FAIL: Book return failed."
    cat "$TMPDIR/session1.log"
    exit 1
}

grep -q "Clean Code" "$TMPDIR/session1.log" || {
    echo "FAIL: Search did not find Clean Code."
    cat "$TMPDIR/session1.log"
    exit 1
}

echo "  [PASS] Session 1: Book added, member added, circulation completed, searched."

echo "=== Running Menu Integration Test (Session 2: Persistence Reload) ==="
# Launch again against the same DATA_DIR to verify records were persisted
"$PROG" << 'EOF' > "$TMPDIR/session2.log" 2>&1
1
2

7
2
2

7
5
EOF

grep -q "Loaded 1 books record(s)" "$TMPDIR/session2.log" || {
    echo "FAIL: Books were not reloaded from persistent storage."
    cat "$TMPDIR/session2.log"
    exit 1
}

grep -q "Clean Code" "$TMPDIR/session2.log" || {
    echo "FAIL: Clean Code not found in reloaded book list."
    cat "$TMPDIR/session2.log"
    exit 1
}

grep -q "Grace Hopper" "$TMPDIR/session2.log" || {
    echo "FAIL: Grace Hopper not found in reloaded member list."
    cat "$TMPDIR/session2.log"
    exit 1
}

echo "  [PASS] Session 2: Verified persistence reload of books and members."
echo "=== All Menu Integration Tests Passed Successfully! ==="
