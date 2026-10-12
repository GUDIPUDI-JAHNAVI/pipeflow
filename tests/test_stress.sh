
#!/usr/bin/env bash
set -euo pipefail

PIPEFLOW="./build/pipeflow"

echo "Running PipeFlow stress tests..."

# Test 1: Three-stage pipeline
result=$("$PIPEFLOW" -- printf 'apple\nbanana\napricot\n' ::: grep '^a' ::: wc -l)
[[ "$result" -eq 2 ]]
echo "PASS: Three-stage pipeline"

# Test 2: Large input (10,000 lines)
result=$(seq 1 10000 | "$PIPEFLOW" -- cat ::: wc -l)
[[ "$result" -eq 10000 ]]
echo "PASS: 10,000-line input"

# Test 3: Larger input (100,000 lines)
result=$(seq 1 100000 | "$PIPEFLOW" -- cat ::: wc -l)
[[ "$result" -eq 100000 ]]
echo "PASS: 100,000-line input"

# Test 4: Failing stage must produce a failure status
if "$PIPEFLOW" -- printf 'hello\n' ::: sh -c 'exit 7' >/dev/null 2>&1; then
    echo "FAIL: Pipeline accepted a failing stage"
    exit 1
fi
echo "PASS: Nonzero stage exit detected"

# Test 5: Missing command must fail
if "$PIPEFLOW" -- nonexistent_command >/dev/null 2>&1; then
    echo "FAIL: Missing command was accepted"
    exit 1
fi
echo "PASS: Missing command rejected"

echo "All PipeFlow stress tests passed."
