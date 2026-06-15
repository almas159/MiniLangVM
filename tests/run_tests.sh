#!/usr/bin/env bash

set -u

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT_DIR/build/minilang"

if [ ! -x "$BIN" ]; then
    echo "error: compiler binary not found: $BIN"
    echo "run: cmake --build build"
    exit 1
fi

passed=0
failed=0

run_valid() {
    local file="$1"

    echo "[VALID] $file"

    "$BIN" "$file" > /tmp/minilang_test_out.txt 2> /tmp/minilang_test_err.txt
    local code=$?

    if [ "$code" -eq 0 ]; then
        echo "  OK"
        passed=$((passed + 1))
    else
        echo "  FAIL: expected success, got exit code $code"
        echo "  stderr:"
        cat /tmp/minilang_test_err.txt
        failed=$((failed + 1))
    fi
}

run_invalid() {
    local file="$1"

    echo "[INVALID] $file"

    "$BIN" "$file" > /tmp/minilang_test_out.txt 2> /tmp/minilang_test_err.txt
    local code=$?

    if [ "$code" -ne 0 ]; then
        echo "  OK"
        passed=$((passed + 1))
    else
        echo "  FAIL: expected error, got success"
        echo "  stdout:"
        cat /tmp/minilang_test_out.txt
        failed=$((failed + 1))
    fi
}

echo "Running valid tests..."
for file in "$ROOT_DIR"/tests/valid/*.ml; do
    run_valid "$file"
done

echo
echo "Running invalid tests..."
for file in "$ROOT_DIR"/tests/invalid/*.ml; do
    run_invalid "$file"
done

echo
echo "Passed: $passed"
echo "Failed: $failed"

if [ "$failed" -ne 0 ]; then
    exit 1
fi

exit 0
