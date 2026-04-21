#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: bash scripts/build_flash.sh [options]

Options:
  --sdk-root PATH       GD32 SDK root, default: /home/fristonp/Simens
  --build-dir NAME      Build directory relative to project root, default: build
  --log-dir NAME        Log directory relative to project root, default: logs
  --build-only          Configure and build only, skip flashing
  --flash-only          Flash only, skip configure/build
  --skip-configure      Skip CMake configure and only build existing build tree
  -h, --help            Show this help

Examples:
  bash scripts/build_flash.sh
  bash scripts/build_flash.sh --build-only
  bash scripts/build_flash.sh --sdk-root /home/fristonp/Simens --build-dir build
EOF
}

SDK_ROOT="/home/fristonp/Simens"
BUILD_DIR="build"
LOG_DIR="logs"
DO_BUILD=1
DO_FLASH=1
SKIP_CONFIGURE=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --sdk-root)
      SDK_ROOT="${2:?missing value for --sdk-root}"
      shift 2
      ;;
    --build-dir)
      BUILD_DIR="${2:?missing value for --build-dir}"
      shift 2
      ;;
    --log-dir)
      LOG_DIR="${2:?missing value for --log-dir}"
      shift 2
      ;;
    --build-only)
      DO_FLASH=0
      shift
      ;;
    --flash-only)
      DO_BUILD=0
      shift
      ;;
    --skip-configure)
      SKIP_CONFIGURE=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

if [[ $DO_BUILD -eq 0 && $DO_FLASH -eq 0 ]]; then
  echo "Nothing to do: both build and flash are disabled." >&2
  exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_PATH="$PROJECT_ROOT/$BUILD_DIR"
LOG_PATH="$PROJECT_ROOT/$LOG_DIR"
PROJECT_NAME="gd32f450_template"
ELF_PATH="$BUILD_PATH/${PROJECT_NAME}.elf"
OPENOCD_CFG="$PROJECT_ROOT/openocd_stlink.cfg"
TIMESTAMP="$(date '+%Y%m%d_%H%M%S')"

mkdir -p "$LOG_PATH"

BUILD_LOG="$LOG_PATH/build_${TIMESTAMP}.log"
FLASH_LOG="$LOG_PATH/flash_${TIMESTAMP}.log"
RUN_LOG="$LOG_PATH/run_${TIMESTAMP}.log"
LATEST_LOG="$LOG_PATH/latest.log"

touch "$RUN_LOG"
ln -sfn "$(basename "$RUN_LOG")" "$LATEST_LOG"

log() {
  local message="[$(date '+%F %T')] $*"
  echo "$message" | tee -a "$RUN_LOG"
}

run_and_log() {
  local stage_name="$1"
  local stage_log="$2"
  shift 2

  log "START ${stage_name}: $*"
  {
    echo "[$(date '+%F %T')] START ${stage_name}"
    echo "COMMAND: $*"
    local exit_code
    if "$@"; then
      exit_code=0
    else
      exit_code=$?
    fi
    echo "[$(date '+%F %T')] END ${stage_name} (exit=${exit_code})"
    exit "$exit_code"
  } 2>&1 | tee -a "$stage_log" | tee -a "$RUN_LOG"

  local cmd_status=${PIPESTATUS[0]}
  if [[ $cmd_status -ne 0 ]]; then
    log "FAIL ${stage_name}: exit=${cmd_status}, log=${stage_log}"
    exit "$cmd_status"
  fi

  log "DONE ${stage_name}: log=${stage_log}"
}

if [[ $DO_BUILD -eq 1 ]]; then
  run_and_log "sdk-check" "$BUILD_LOG" bash "$SCRIPT_DIR/check_gd32_sdk.sh" "$SDK_ROOT"

  if [[ $SKIP_CONFIGURE -eq 0 ]]; then
    run_and_log \
      "configure" \
      "$BUILD_LOG" \
      env -u CC -u CXX -u CFLAGS -u CXXFLAGS -u CPPFLAGS -u LDFLAGS \
      cmake --fresh -S "$PROJECT_ROOT" -B "$BUILD_PATH" -G Ninja \
      -DCMAKE_TOOLCHAIN_FILE="$PROJECT_ROOT/cmake/toolchain-arm-none-eabi.cmake" \
      -DGD32_SDK_ROOT="$SDK_ROOT"
  else
    log "SKIP configure stage"
  fi

  run_and_log "build" "$BUILD_LOG" cmake --build "$BUILD_PATH"
fi

if [[ $DO_FLASH -eq 1 ]]; then
  if [[ ! -f "$ELF_PATH" ]]; then
    log "Missing firmware image: $ELF_PATH"
    exit 1
  fi

  if ! command -v openocd >/dev/null 2>&1; then
    log "openocd not found in PATH"
    exit 1
  fi

  run_and_log \
    "flash" \
    "$FLASH_LOG" \
    openocd -f "$OPENOCD_CFG" \
    -c "program $ELF_PATH verify reset exit"
fi

log "All requested stages completed successfully."
log "Combined log: $RUN_LOG"
if [[ $DO_BUILD -eq 1 ]]; then
  log "Build log: $BUILD_LOG"
fi
if [[ $DO_FLASH -eq 1 ]]; then
  log "Flash log: $FLASH_LOG"
fi
