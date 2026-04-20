#!/usr/bin/env bash
set -euo pipefail

SDK_ROOT="${1:-/home/fristonp/Simens}"

required=(
  "$SDK_ROOT/Firmware/CMSIS/GD/GD32F4xx/Source/GCC/Ld/gd32f450xI_flash.ld"
  "$SDK_ROOT/Firmware/CMSIS/GD/GD32F4xx/Source/GCC/startup_gd32f450_470.S"
  "$SDK_ROOT/Firmware/CMSIS/GD/GD32F4xx/Source/system_gd32f4xx.c"
  "$SDK_ROOT/Utilities/gd32f450i_eval.c"
)

missing=0
for f in "${required[@]}"; do
  if [[ ! -f "$f" ]]; then
    echo "MISSING: $f"
    missing=1
  fi
done

if [[ $missing -ne 0 ]]; then
  echo
  echo "GD32 SDK is incomplete."
  echo "Please place the official GD32F4xx firmware library so that:"
  echo "  $SDK_ROOT/Firmware"
  echo "  $SDK_ROOT/Utilities"
  echo "exist."
  exit 1
fi

echo "GD32 SDK check passed at: $SDK_ROOT"
