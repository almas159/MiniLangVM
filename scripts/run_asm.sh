#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"

if [ "$#" -lt 1 ]; then
    echo "Usage:"
    echo "  scripts/run_asm.sh <source.ml> [output-executable]"
    echo
    echo "Example:"
    echo "  scripts/run_asm.sh examples/asm_array.ml"
    echo "  scripts/run_asm.sh examples/asm_array.ml build/asm_array"
    exit 1
fi

SOURCE="$1"

if [ "$#" -ge 2 ]; then
    OUT="$2"
else
    BASENAME="$(basename "$SOURCE" .ml)"
    OUT="$BUILD_DIR/$BASENAME"
fi

"$ROOT_DIR/scripts/build_asm.sh" "$SOURCE" "$OUT"

echo
echo "[run] $OUT"
echo "----------------------------------------"

set +e
"$OUT"
CODE=$?
set -e

echo "----------------------------------------"
echo "exit code: $CODE"

exit "$CODE"
