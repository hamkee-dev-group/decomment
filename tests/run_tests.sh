#!/bin/sh
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "Building tests..."
cd "$PROJECT_DIR"

CC="${CC:-cc}"
CFLAGS="-std=c17 -Wall -Wextra -Wpedantic -I include -I src"

# Compile test binary
$CC $CFLAGS -o tests/test_runner \
    tests/test_main.c \
    src/buf.c \
    src/handlers/c_like.c \
    src/handlers/python.c \
    src/handlers/shell.c \
    src/handlers/hash.c \
    src/handlers/lua.c \
    -DTESTING

echo "Running tests..."
tests/test_runner

echo ""
echo "All tests completed."
