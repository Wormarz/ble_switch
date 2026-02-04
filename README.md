# BLE Switch

Battery-powered BLE remote mechanical switch (Rust app logic + Zephyr C BLE/hardware). See [design.md](design.md) for requirements.

## Prerequisites

- **Python 3** with `venv`: `sudo apt install python3-venv python3-pip`
- **Rust** (rustup): <https://rustup.rs/> — add target: `rustup target add thumbv7em-none-eabihf`
- **Toolchain** (one of):
  - **GNU Arm Embedded** (no SDK): `sudo apt install gcc-arm-none-eabi` then set `ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb` when building
  - **Zephyr SDK**: install [Zephyr SDK](https://docs.zephyrproject.org/latest/develop/getting_started/index.html) and set `ZEPHYR_SDK_INSTALL_DIR` to its path

## Zephyr (in-tree)

Zephyr source, `.west`, and modules live **only under `zephyr/`**. Run the script from project root so the workspace is not created at the repo root.

1. From project root, run:
   ```bash
   ./scripts/setup_zephyr.sh
   ```
   The script excludes optional modules (lvgl, nanopb, trusted-firmware-a) to avoid update failures; only cmsis and other nRF52-needed modules are fetched.

2. For builds, activate the venv and set Zephyr + toolchain (from project root):
   ```bash
   source zephyr/.venv/bin/activate
   export ZEPHYR_BASE=$(pwd)/zephyr/zephyr
   # Use system ARM GCC (no Zephyr SDK):
   export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
   # Or if you use Zephyr SDK: export ZEPHYR_SDK_INSTALL_DIR=/path/to/zephyr-sdk-0.16.x
   ```

## Build

1. Build the Rust static library:
   ```bash
   cd rust && cargo build --release --target thumbv7em-none-eabihf && cd ..
   ```
2. Build the Zephyr application (from project root, with venv active and env vars above set):
   ```bash
   cd zephyr && west build -b nrf52840dk_nrf52840 ..
   ```
   **Using system ARM GCC (no Zephyr SDK):**
   ```bash
   sudo apt install gcc-arm-none-eabi   # Debian/Ubuntu
   source zephyr/.venv/bin/activate
   source scripts/env.sh               # sets ZEPHYR_BASE and ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
   cd zephyr && west build -b nrf52840dk_nrf52840 ..
   ```

## Flash

```bash
west flash
```
(Connect nRF52840 DK via USB; udev rules may be required on Linux.)

## Layout

- **`rust/`** — Application logic (state machine, storage, motor/UI/power policy); built as staticlib, called from C via FFI.
- **`src/`** — C: main, BLE GATT service, hw_glue (motor/LED/NVS/battery), button, timer_glue.
- **`zephyr/`** — Zephyr repo and (optionally) SDK; created by `scripts/setup_zephyr.sh`.
- **`boards/`** — Board overlays (e.g. nRF52840 DK).

## GATT

- **Remote Mechanical Switch Service** (128-bit UUID).
- **Switch Control**: read = current logical state (0/1); write = 0 off, 1 on, 2 toggle.

## Optional / TODO

- **NVS**: `storage_read`/`storage_write` in `hw_glue.c` are stubs; add Zephyr NVS and a partition to persist logical state across reboot.
- **Battery**: `battery_read_percent()` is stub (returns 100); add ADC sampling and scaling for real battery level.
- **Motor**: `motor_move_to_on`/`motor_move_to_off` are stubs; add PWM (e.g. servo on a PWM pin) in `hw_glue.c`.
