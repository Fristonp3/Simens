#!/usr/bin/env bash
set -euo pipefail

sudo apt-get update
sudo apt-get install -y \
  cmake \
  ninja-build \
  gcc-arm-none-eabi \
  gdb-multiarch \
  openocd

echo "Environment setup complete."
cmake --version
ninja --version
arm-none-eabi-gcc --version | head -n 1
openocd --version | head -n 1
