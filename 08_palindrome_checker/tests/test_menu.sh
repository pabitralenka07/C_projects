#!/usr/bin/env bash
set -euo pipefail

PROG="${1:-build/palindrome}"
if [ ! -x "$PROG" ]; then
    echo "Error: Binary '$PROG' not found or not executable."
    exit 1
fi

TMPDIR=$(mktemp -d /tmp/pal_test_XXXXXX)
trap 'rm -rf "$TMPDIR"' EXIT

echo "=== Running Palindrome Menu Integration Test ==="

# Stdin commands:
# 1 (Check String) -> "Never odd or even" -> y (relaxed) -> Enter
# 2 (Check Int) -> 1234321 -> Enter
# 3 (Longest Substring) -> "banana" -> Enter
# 4 (Famous Examples) -> Enter
# 5 (Exit)
"$PROG" << 'EOF' > "$TMPDIR/session.log" 2>&1
1
Never odd or even
y

2
1234321

3
banana

4

5
EOF

grep -q "is a PALINDROME! ✅" "$TMPDIR/session.log" || {
    echo "FAIL: Expected 'Never odd or even' to be recognized as palindrome."
    cat "$TMPDIR/session.log"
    exit 1
}

grep -q "1234321 is a PALINDROMIC NUMBER! ✅" "$TMPDIR/session.log" || {
    echo "FAIL: Expected 1234321 to be recognized as palindromic number."
    cat "$TMPDIR/session.log"
    exit 1
}

grep -q "anana" "$TMPDIR/session.log" || {
    echo "FAIL: Expected longest palindromic substring 'anana' in 'banana'."
    cat "$TMPDIR/session.log"
    exit 1
}

echo "  [PASS] Palindrome menu integration test passed!"
