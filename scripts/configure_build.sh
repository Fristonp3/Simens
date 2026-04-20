#!/usr/bin/env bash
set -euo pipefail

SDK_ROOT="${1:-/home/fristonp/Simens}"
BUILD_DIR="${2:-build}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

"$SCRIPT_DIR/check_gd32_sdk.sh" "$SDK_ROOT"

env -u CC -u CXX -u CFLAGS -u CXXFLAGS -u CPPFLAGS -u LDFLAGS \
  cmake --fresh -S "$PROJECT_ROOT" -B "$PROJECT_ROOT/$BUILD_DIR" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$PROJECT_ROOT/cmake/toolchain-arm-none-eabi.cmake" \
  -DGD32_SDK_ROOT="$SDK_ROOT"

cmake --build "$PROJECT_ROOT/$BUILD_DIR"
