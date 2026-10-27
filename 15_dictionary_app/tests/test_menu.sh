#!/usr/bin/env bash
# Integration test for Dictionary App
set -euo pipefail

PROG="${1:-build/dict}"
if [ ! -x "$PROG" ]; then
    echo "Error: Binary '$PROG' not found or not executable."
    exit 1
fi

TMPDIR=$(mktemp -d /tmp/dict_menu_test_XXXXXX)
trap 'rm -rf "$TMPDIR"' EXIT

export DATA_DIR="$TMPDIR"

echo "=== Running Dictionary App Menu Integration Test ==="

# Test 1: Immediate clean exit on EOF
echo "--- Testing clean exit on EOF (< /dev/null) ---"
if "$PROG" < /dev/null > "$TMPDIR/eof_test.log" 2>&1; then
    echo "  [PASS] Program exited cleanly on EOF (< /dev/null)"
else
    echo "  [FAIL] Program failed to exit cleanly on EOF!"
    exit 1
fi

# Test 2: Add word, exact lookup, prefix search, word of the day, delete, exit
echo "--- Testing interactive vocabulary operations ---"
"$PROG" << 'EOF' > "$TMPDIR/session1.log" 2>&1
5
cache
noun
High-speed memory buffer
CPU L1 cache

1
cache

2
ca

4

7
cache
y

9
EOF

grep -q "Word 'cache' added with ID" "$TMPDIR/session1.log" || (echo "Add word failed"; exit 1)
grep -q "High-speed memory buffer" "$TMPDIR/session1.log" || (echo "Exact lookup failed"; exit 1)
grep -q "Found 1 words with prefix 'ca'" "$TMPDIR/session1.log" || (echo "Prefix search failed"; exit 1)
grep -q "Word of the Day" "$TMPDIR/session1.log" || (echo "Word of the day failed"; exit 1)
grep -q "Word 'cache' has been removed" "$TMPDIR/session1.log" || (echo "Delete word failed"; exit 1)
echo "  [PASS] Session: Add, binary search lookup, prefix search, word of the day, delete succeeded."

echo "=== All Dictionary App Integration Tests Passed Successfully! ==="
