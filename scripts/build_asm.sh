#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
COMPILER="$BUILD_DIR/minilang"

if [ "$#" -lt 1 ]; then
    echo "Usage:"
    echo "  scripts/build_asm.sh <source.ml> [output-executable]"
    echo
    echo "Example:"
    echo "  scripts/build_asm.sh examples/asm_array.ml"
    echo "  scripts/build_asm.sh examples/asm_array.ml build/asm_array"
    exit 1
fi

SOURCE="$1"

if [ ! -f "$SOURCE" ]; then
    echo "error: source file not found: $SOURCE"
    exit 1
fi

if [ ! -x "$COMPILER" ]; then
    echo "compiler not found, building project first..."
    cmake --build "$BUILD_DIR"
fi

if ! command -v nasm >/dev/null 2>&1; then
    echo "error: nasm not found"
    echo "install it with:"
    echo "  sudo apt install nasm"
    exit 1
fi

if [ "$#" -ge 2 ]; then
    OUT="$2"
else
    BASENAME="$(basename "$SOURCE" .ml)"
    OUT="$BUILD_DIR/$BASENAME"
fi

ASM_FILE="$OUT.asm"
OBJ_FILE="$OUT.o"
EXE_FILE="$OUT"

mkdir -p "$(dirname "$OUT")"

echo "[1/4] Generating assembly:"
echo "      $ASM_FILE"
"$COMPILER" "$SOURCE" --emit-asm -o "$ASM_FILE"

echo "[2/4] Assembling:"
echo "      $OBJ_FILE"
nasm -f elf64 "$ASM_FILE" -o "$OBJ_FILE"

echo "[3/4] Linking:"
echo "      $EXE_FILE"
gcc -no-pie "$OBJ_FILE" -o "$EXE_FILE"

echo "[4/4] Done"
echo
echo "Executable:"
echo "  $EXE_FILE"
echo
echo "Run:"
echo "  $EXE_FILE"
