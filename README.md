# BLE Switch

Battery-powered BLE remote mechanical switch (Rust app logic + Zephyr C BLE/hardware). See [design.md](design.md) for requirements.

## Prerequisites

- DTC
- Ninja
- **Python 3** with `venv`: `sudo apt install python3-venv python3-pip`
- **Rust** (rustup): <https://rustup.rs/> — add target: `rustup target add thumbv7em-none-eabi`
- **Toolchain** (one of):
  - **GNU Arm Embedded** (no SDK): `sudo apt install gcc-arm-none-eabi` then set `ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb` when building
  - **Zephyr SDK**: install [Zephyr SDK](https://docs.zephyrproject.org/latest/develop/getting_started/index.html) and set `ZEPHYR_SDK_INSTALL_DIR` to its path

## Zephyr (in-tree)

Zephyr source, `.west`, and modules live **only under `zephyr/`**. Run the script from project root so the workspace is not created at the repo root.

1. From project root, run:
   ```bash
   ./scripts/setup_zephyr.sh
   ```
   This creates a venv under `zephyr/`, does a shallow Zephyr clone with retry logic, runs `west init -l zephyr` + `west update` (with retries), and installs Zephyr Python dependencies. If CMSIS is still missing for nRF52 builds, rerun `west update` inside `zephyr/` after fixing any network issues.

2. For builds, activate the venv and set Zephyr + toolchain (from project root):
   ```bash
   source zephyr/.venv/bin/activate
   export ZEPHYR_BASE=$(pwd)/zephyr/zephyr
   # Use system ARM GCC (no Zephyr SDK):
   export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
   # Or if you use Zephyr SDK: export ZEPHYR_SDK_INSTALL_DIR=/path/to/zephyr-sdk-0.16.x
   ```

## Build

1. Install ARM toolchain if needed: `sudo apt install gcc-arm-none-eabi` (Debian/Ubuntu).

2. Build the Zephyr application (from project root, Rust is built via CMake custom command):
   ```bash
   ./scripts/build.sh
   ```
   The script uses the **system ARM GCC** by default. If `ZEPHYR_SDK_INSTALL_DIR` is set (or the SDK was auto-installed under `zephyr/sdk`), it uses the **Zephyr SDK** toolchain instead. Optional: `BOARD=nrf52840dk/nrf52840 ./scripts/build.sh` or `./scripts/build.sh --pristine`.

3. (Optional) Build only the Rust static library:
   ```bash
   cd rust
   cargo build --release --target thumbv7em-none-eabi
   cd ..
   ```

### Using the Zephyr SDK toolchain

1. **Install Zephyr SDK** (Linux x86_64 example):
   - Download from `https://github.com/zephyrproject-rtos/sdk-ng/releases`, pick a version compatible with your Zephyr (e.g. `zephyr-sdk-0.16.x_linux-x86_64.tar.xz`).
   - Extract to any directory (for example, inside this project):
     ```bash
     mkdir -p zephyr/sdk
     tar -xJf zephyr-sdk-0.16.*_linux-x86_64.tar.xz -C zephyr/sdk --strip-components=1
     ```
   - Run the SDK setup script (installs toolchains, QEMU, etc.):
     ```bash
     zephyr/sdk/setup.sh
     ```
   - When prompted, select architectures to install (for nRF52, select ARM).

2. **Use SDK for builds**: set `ZEPHYR_SDK_INSTALL_DIR` and run the build script:
   ```bash
   export ZEPHYR_SDK_INSTALL_DIR=$(pwd)/zephyr/sdk    # if SDK is inside the project
   # or: export ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk-0.16.0
   ./scripts/build.sh
   ```
   You can also hardcode `export ZEPHYR_SDK_INSTALL_DIR=...` in `~/.bashrc` or `scripts/env.sh`, then simply run `./scripts/build.sh` and it will automatically use the SDK.

## Flash

```bash
west flash
```
(Connect nRF52840 DK via USB; udev rules may be required on Linux.)

## Layout

- **`rust/`** — Application logic (state machine, storage, motor/UI/power policy); built as staticlib, called from C via FFI.
- **`src/`** — C: main, BLE GATT service, hw_glue (motor/LED/NVS/battery), button, timer_glue.
- **`zephyr/`** — Zephyr repo, modules, and (optionally) SDK; created by `scripts/setup_zephyr.sh`.
- **`boards/`** — Board overlays (e.g. nRF52840 DK).
- **`bootloader/` (root)** — Ignored by git; legacy location if an earlier MCUboot clone existed. MCUboot managed by west should live under `zephyr/bootloader/mcuboot`.

## Notes / Troubleshooting

- **Rust/Zephyr ABI**: Rust is built for `thumbv7em-none-eabi` (soft-float) to match Zephyr’s default ABI on nRF52840. Make sure `rustup target add thumbv7em-none-eabi` is installed.
- **Kconfig warnings**: `zephyr/zephyr/scripts/kconfig/kconfig.py` is patched locally to avoid treating Kconfig warnings as fatal. Updating Zephyr may overwrite this patch; re-apply if builds start aborting due to warnings again.
- **Network issues (`git`/`west`)**: `scripts/setup_zephyr.sh` and `west update` may fail with TLS/curl errors on poor networks. Simply rerun the script or `west update` from `zephyr/` once the network is stable.
- **MCUboot**: MCUboot should be fetched under `zephyr/bootloader/mcuboot` via `cd zephyr && .venv/bin/west update mcuboot`. If this fails due to network errors, retry later; the root `bootloader/` directory is kept out of git via `.gitignore`.

## GATT

- **Remote Mechanical Switch Service** (128-bit UUID).
- **Switch Control**: read = current logical state (0/1); write = 0 off, 1 on, 2 toggle.

## Optional / TODO

- **NVS**: `storage_read`/`storage_write` in `hw_glue.c` are stubs; add Zephyr NVS and a partition to persist logical state across reboot.
- **Battery**: `battery_read_percent()` is stub (returns 100); add ADC sampling and scaling for real battery level.
- **Motor**: `motor_move_to_on`/`motor_move_to_off` are stubs; add PWM (e.g. servo on a PWM pin) in `hw_glue.c`.
