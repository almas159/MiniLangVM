#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"

echo "========================================"
echo "MiniLangVM full project check"
echo "========================================"
echo

echo "[1/4] Configuring CMake"
cmake -S "$ROOT_DIR" -B "$BUILD_DIR"

echo
echo "[2/4] Building compiler"
cmake --build "$BUILD_DIR"

echo
echo "[3/4] Running VM backend tests"
"$ROOT_DIR/tests/run_tests.sh"

echo
echo "[4/4] Running x86-64 asm backend tests"
"$ROOT_DIR/scripts/test_asm.sh"

echo
echo "========================================"
echo "All checks passed"
echo "========================================"
