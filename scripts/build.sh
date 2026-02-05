#!/usr/bin/env bash
# Build the BLE Switch app. Run from project root: ./scripts/build.sh
# Uses Zephyr SDK if ZEPHYR_SDK_INSTALL_DIR is set, otherwise system gnuarmemb.
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BOARD="${BOARD:-nrf52840dk/nrf52840}"

export ZEPHYR_BASE="$PROJECT_ROOT/zephyr/zephyr"
if [ -z "$ZEPHYR_SDK_INSTALL_DIR" ] && [ -f "$PROJECT_ROOT/zephyr/sdk/sdk_version" ]; then
  export ZEPHYR_SDK_INSTALL_DIR="$PROJECT_ROOT/zephyr/sdk"
fi
if [ -n "$ZEPHYR_SDK_INSTALL_DIR" ]; then
  export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
  echo "Using Zephyr SDK: $ZEPHYR_SDK_INSTALL_DIR"
else
  export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
  echo "Using system toolchain (gnuarmemb). Set ZEPHYR_SDK_INSTALL_DIR to use Zephyr SDK."
fi

cd "$PROJECT_ROOT"
if [ ! -f "zephyr/.venv/bin/activate" ]; then
  echo "Run ./scripts/setup_zephyr.sh first."
  exit 1
fi
source zephyr/.venv/bin/activate
cd zephyr
west build -b "$BOARD" "$@" ..
echo "Build done. Flash with: cd zephyr && west flash"
