#!/usr/bin/env bash

set -u

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"

passed=0
failed=0

run_asm_valid() {
    local file="$1"
    local name
    name="$(basename "$file" .ml)"
    local out="$BUILD_DIR/asm_test_$name"

    echo "[ASM VALID] $file"

    "$ROOT_DIR/scripts/build_asm.sh" "$file" "$out" >/tmp/minilang_asm_build_out.txt 2>/tmp/minilang_asm_build_err.txt
    local build_code=$?

    if [ "$build_code" -ne 0 ]; then
        echo "  FAIL: asm build failed"
        echo "  stderr:"
        cat /tmp/minilang_asm_build_err.txt
        failed=$((failed + 1))
        return
    fi

    "$out" >/tmp/minilang_asm_run_out.txt 2>/tmp/minilang_asm_run_err.txt
    local run_code=$?

    if [ "$run_code" -eq 0 ]; then
        echo "  OK"
        passed=$((passed + 1))
    else
        echo "  FAIL: expected exit code 0, got $run_code"
        echo "  stdout:"
        cat /tmp/minilang_asm_run_out.txt
        echo "  stderr:"
        cat /tmp/minilang_asm_run_err.txt
        failed=$((failed + 1))
    fi
}

echo "Running ASM valid tests..."

for file in "$ROOT_DIR"/tests/asm/valid/*.ml; do
    run_asm_valid "$file"
done

echo
echo "ASM Passed: $passed"
echo "ASM Failed: $failed"

if [ "$failed" -ne 0 ]; then
    exit 1
fi

exit 0
