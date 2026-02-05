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

1. Install ARM toolchain if needed: `sudo apt install gcc-arm-none-eabi` (Debian/Ubuntu).

2. Build the Rust static library:
   ```bash
   cd rust && cargo build --release --target thumbv7em-none-eabihf && cd ..
   ```

3. Build the Zephyr application (from project root):
   ```bash
   ./scripts/build.sh
   ```
   The script uses the **system ARM GCC** by default. If `ZEPHYR_SDK_INSTALL_DIR` is set, it uses the **Zephyr SDK** toolchain instead. Optional: `BOARD=nrf52840dk/nrf52840 ./scripts/build.sh` or `./scripts/build.sh --pristine`.

### 使用 Zephyr SDK 工具链

1. **安装 Zephyr SDK**（以 Linux x86_64 为例）：
   - 下载：<https://github.com/zephyrproject-rtos/sdk-ng/releases>，选择与 Zephyr 版本匹配的 SDK（例如 `zephyr-sdk-0.16.x_linux-x86_64.tar.xz`）。
   - 解压到任意目录，例如工程内：
     ```bash
     mkdir -p zephyr/sdk
     tar -xJf zephyr-sdk-0.16.*_linux-x86_64.tar.xz -C zephyr/sdk --strip-components=1
     ```
   - 运行 SDK 安装脚本（安装工具链、QEMU 等）：
     ```bash
     zephyr/sdk/setup.sh
     ```
   - 按提示选择要安装的架构（nRF52 选 ARM）。

2. **构建时使用 SDK**：设置 `ZEPHYR_SDK_INSTALL_DIR` 后执行构建脚本即可：
   ```bash
   export ZEPHYR_SDK_INSTALL_DIR=$(pwd)/zephyr/sdk    # 若 SDK 在工程内
   # 或 export ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk-0.16.0
   ./scripts/build.sh
   ```
   也可在 `~/.bashrc` 或 `scripts/env.sh` 中写死 `export ZEPHYR_SDK_INSTALL_DIR=...`，之后直接 `./scripts/build.sh` 就会用 SDK。

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
