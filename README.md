# BLE Switch

Battery-powered BLE remote mechanical switch: Rust state machine and storage logic, Zephyr C for BLE, motor, NVS, button, and ADC. Target board nRF5340 DK application core (or use another board overlay).

## Implemented features

- **BLE GATT**
  - Custom **Remote Mechanical Switch Service** (128-bit UUID): Switch Control (read logical state; write 0=off, 1=on, 2=toggle) and Mapping (read/write logical↔physical mapping, persisted in NVS).
  - Standard **Battery Service** (0x180F): battery level 0–100%.
  - **Status/error** characteristic: 0=Idle, 1=MovingToOn, 2=MovingToOff, 3=Error.
  - Optional fixed passkey pairing when `CONFIG_BT_APP_PASSKEY` is set.
- **State model**
  - **Physical state** (motor position 0 or 1): loaded from NVS on boot; updated in RAM when motion completes. Not written to NVS on every change—only when the **save-state GPIO** goes low (reduces flash wear).
  - **Mapping** (0=normal, 1=inverted): configurable via BLE, persisted in NVS. Logical on/off = f(physical_state, mapping).
  - **Logical state**: user-visible on/off; read over BLE and used for toggle/set.
- **NVS**
  - Two keys: physical state (written only on save-state trigger) and mapping (written when set via BLE). Factory reset clears both and removes BLE bonds.
- **Motor**
  - GPIO H-bridge (pins 13/14 on GPIO0): move to ON, move to OFF, stop. Motion timeout (e.g. 1.5 s) stops motor and updates in-memory physical state.
- **Button** (devicetree `sw0`)
  - Short press: toggle logical switch.
  - Long press (2 s): enter pairing / factory reset (clear NVS, unpair all BLE bonds).
- **Battery**
  - ADC via `zephyr,user` io-channels when configured; 0–100% from voltage curve. Motor disabled when level ≤ 10%. Level exposed over BLE.
- **LED** (devicetree `led0` if present)
  - Short flashes: pairing, feedback (motion done), error (e.g. low battery on move).

## Prerequisites

- DTC
- Ninja
- **Python 3** with `venv`: `sudo apt install python3-venv python3-pip`
- **Rust** (rustup): <https://rustup.rs/> — add target: `rustup target add thumbv8m.main-none-eabihf`
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
   The script uses the **system ARM GCC** by default. If `ZEPHYR_SDK_INSTALL_DIR` is set (or the SDK was auto-installed under `zephyr/sdk`), it uses the **Zephyr SDK** toolchain instead. Optional: `BOARD=nrf5340dk/nrf5340/cpuapp ./scripts/build.sh` or `./scripts/build.sh --pristine`.

3. (Optional) Build only the Rust static library (nRF5340 app core, Cortex-M33F, hard-float ABI to match Zephyr):
   ```bash
   cd rust
   cargo build --release --target thumbv8m.main-none-eabihf
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
(Connect nRF5340 DK via USB; udev rules may be required on Linux.)

## Layout

- **`rust/`** — Application logic (state machine, storage, motor/UI/power policy); built as staticlib, called from C via FFI.
- **`src/`** — C: main, BLE GATT service, hw_glue (motor/LED/NVS/battery), button, timer_glue.
- **`zephyr/`** — Zephyr repo, modules, and (optionally) SDK; created by `scripts/setup_zephyr.sh`.
- **`boards/`** — Board overlays (e.g. nRF5340 DK application core).
- **`bootloader/` (root)** — Ignored by git; legacy location if an earlier MCUboot clone existed. MCUboot managed by west should live under `zephyr/bootloader/mcuboot`.

## Notes / Troubleshooting

- **Rust/Zephyr ABI**: Rust is built for `thumbv8m.main-none-eabihf` (Cortex-M33F hard-float) to match Zephyr’s ABI on the nRF5340 application core. Make sure `rustup target add thumbv8m.main-none-eabihf` is installed.
- **Kconfig warnings**: `zephyr/zephyr/scripts/kconfig/kconfig.py` is patched locally to avoid treating Kconfig warnings as fatal. Updating Zephyr may overwrite this patch; re-apply if builds start aborting due to warnings again.
- **Network issues (`git`/`west`)**: `scripts/setup_zephyr.sh` and `west update` may fail with TLS/curl errors on poor networks. Simply rerun the script or `west update` from `zephyr/` once the network is stable.
- **MCUboot**: MCUboot should be fetched under `zephyr/bootloader/mcuboot` via `cd zephyr && .venv/bin/west update mcuboot`. If this fails due to network errors, retry later; the root `bootloader/` directory is kept out of git via `.gitignore`.

## GATT summary

| Service / characteristic | Description |
|--------------------------|-------------|
| **Remote Mechanical Switch** (custom 128-bit) | |
| Switch Control | Read: logical state (0/1). Write: 0=off, 1=on, 2=toggle. |
| Mapping (orientation) | Read/write: 0=normal (physical 1⇒logical on), 1=inverted. Persisted in NVS. |
| **Battery** (0x180F) | Battery level 0–100%. |
| **Status** (Device Info namespace) | 0=Idle, 1=MovingToOn, 2=MovingToOff, 3=Error. |

**Save-state trigger**: When GPIO `SAVE_STATE_TRIGGER_PIN` in `hw_glue.c` (default **P0.15**) goes **low** (falling edge, input pull-up), a work item runs that writes the current physical state to NVS. This avoids writing NVS on every switch change and reduces flash wear.

## Hardware (default / in code)

- **Motor**: GPIO0 pins 13 (IN1), 14 (IN2); H-bridge style ON/OFF/stop.
- **Save-state trigger**: GPIO0 pin 15; change `SAVE_STATE_TRIGGER_PIN` in `hw_glue.c` if needed.
- **Button**: devicetree alias `sw0`.
- **LED**: devicetree alias `led0` (optional).
- **Battery ADC**: `zephyr,user` io-channels in devicetree (optional; else reports 100%).

## Optional / future

- Tune battery voltage curve (e.g. min/max mV) in `hw_glue.c` for your cell.
- Replace GPIO motor drive with PWM/servo in `hw_glue.c` if your hardware requires it.
