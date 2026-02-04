# Source this from project root before building (e.g. source scripts/env.sh).
# Sets ZEPHYR_BASE and toolchain to use system GNU Arm (no Zephyr SDK required).
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
export ZEPHYR_BASE="$PROJECT_ROOT/zephyr/zephyr"
export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
echo "ZEPHYR_BASE=$ZEPHYR_BASE"
echo "ZEPHYR_TOOLCHAIN_VARIANT=$ZEPHYR_TOOLCHAIN_VARIANT (use system arm-none-eabi-gcc)"
echo "Activate venv: source $PROJECT_ROOT/zephyr/.venv/bin/activate"
