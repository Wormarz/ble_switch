#!/usr/bin/env bash
# Build the BLE Switch app. Run from project root: ./scripts/build.sh
# Uses Zephyr SDK if ZEPHYR_SDK_INSTALL_DIR is set, otherwise system gnuarmemb.
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BOARD="${BOARD:-nrf5340dk/nrf5340/cpuapp}"
NET_BUILD_DIR="$PROJECT_ROOT/zephyr/build_cpunet"
NET_HEX="$NET_BUILD_DIR/zephyr/zephyr.hex"

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

# If we are building for the nRF5340 application core, ensure the network core
# (cpunet) has an HCI RPMsg controller image. Build it once if missing.
if [[ "$BOARD" == nrf5340dk/nrf5340/cpuapp* ]]; then
  if [ ! -f "$NET_HEX" ]; then
    echo "Net core image not found, building HCI controller (hci_ipc) for nrf5340dk/nrf5340/cpunet..."
    cd zephyr/zephyr
    west build -b nrf5340dk/nrf5340/cpunet samples/bluetooth/hci_ipc -d ../build_cpunet
    cd "$PROJECT_ROOT"
  else
    echo "Net core image already exists at $NET_HEX, skipping net core build."
  fi
fi

cd zephyr
west build -b "$BOARD" "$@" ..
echo "Build done. Flash with: cd zephyr && west flash"
