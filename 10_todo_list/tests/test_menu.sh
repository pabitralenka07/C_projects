#!/usr/bin/env bash
set -euo pipefail

PROG="${1:-build/todo}"
if [ ! -x "$PROG" ]; then
    echo "Error: Binary '$PROG' not found or not executable."
    exit 1
fi

TMPDIR=$(mktemp -d /tmp/todo_test_XXXXXX)
trap 'rm -rf "$TMPDIR"' EXIT

export DATA_DIR="$TMPDIR"

echo "=== Running To-Do List Menu Integration Test ==="

# Stdin commands:
# 1 (Add Task) -> Title -> Category -> Priority -> Enter
# 2 (List Tasks) -> Enter
# 3 (Toggle Task) -> 1 -> Enter
# 5 (Search) -> "Release" -> Enter
# 8 (Exit)
"$PROG" << 'EOF' > "$TMPDIR/session1.log" 2>&1
1
Prepare release v1.0
Engineering
3

2

3
1

5
Release

8
EOF

grep -q "Task added successfully! (ID: 1)" "$TMPDIR/session1.log" || {
    echo "FAIL: Task was not added."
    cat "$TMPDIR/session1.log"
    exit 1
}

grep -q "Prepare release v1.0" "$TMPDIR/session1.log" || {
    echo "FAIL: Task title not listed."
    cat "$TMPDIR/session1.log"
    exit 1
}

grep -q "Task #1 is now marked as \[COMPLETED\]" "$TMPDIR/session1.log" || {
    echo "FAIL: Task was not marked as completed."
    cat "$TMPDIR/session1.log"
    exit 1
}

# Session 2: Reload and verify persistence
"$PROG" << 'EOF' > "$TMPDIR/session2.log" 2>&1
2

8
EOF

grep -q "\[DONE\]" "$TMPDIR/session2.log" || {
    echo "FAIL: Completed task was not reloaded from file."
    cat "$TMPDIR/session2.log"
    exit 1
}

echo "  [PASS] To-Do list menu integration test passed!"
