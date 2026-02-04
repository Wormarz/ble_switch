#!/usr/bin/env bash
# Install Zephyr source and Python deps under project's zephyr/ directory.
# Prerequisites: python3-venv (e.g. sudo apt install python3-venv python3-pip)
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
ZEPHYR_DIR="$PROJECT_ROOT/zephyr"
cd "$PROJECT_ROOT"
mkdir -p "$ZEPHYR_DIR"

# Ensure west workspace is only under zephyr/, not at project root
if [ -d "$PROJECT_ROOT/.west" ] || [ -d "$PROJECT_ROOT/modules" ] || [ -d "$PROJECT_ROOT/tools" ]; then
  echo "Removing .west/modules/tools from project root (workspace must be under zephyr/) ..."
  rm -rf "$PROJECT_ROOT/.west" "$PROJECT_ROOT/modules" "$PROJECT_ROOT/tools"
fi
cd "$ZEPHYR_DIR"

if [ ! -f ".venv/bin/activate" ]; then
  if [ -d ".venv" ]; then
    echo "Removing incomplete .venv ..."
    rm -rf .venv
  fi
  echo "Creating .venv in $ZEPHYR_DIR ..."
  python3 -m venv .venv
fi
source .venv/bin/activate
pip install -U pip
pip install west

# Clone Zephyr repo into zephyr/zephyr/ so that "west init -l zephyr" creates .west inside zephyr/
if [ ! -f "zephyr/west.yml" ]; then
  if [ -f "west.yml" ]; then
    echo "Moving existing Zephyr repo into $ZEPHYR_DIR/zephyr/ ..."
    rm -rf .west 2>/dev/null || true
    mkdir -p zephyr.tmp
    shopt -s dotglob nullglob
    for f in * .[!.]* ..?*; do
      [ -e "$f" ] || [ -L "$f" ] || continue
      [ "$f" = ".venv" ] && continue
      [ "$f" = "zephyr.tmp" ] && continue
      mv "$f" zephyr.tmp/
    done
    shopt -u dotglob nullglob
    mv zephyr.tmp zephyr
  else
    echo "Cloning Zephyr into $ZEPHYR_DIR/zephyr (shallow) ..."
    rm -rf .west zephyr .west-manifest-tmp 2>/dev/null || true
    git config --global http.postBuffer 524288000 2>/dev/null || true
    for attempt in 1 2 3; do
      if git clone --depth 1 --branch main https://github.com/zephyrproject-rtos/zephyr .west-manifest-tmp; then
        break
      fi
      echo "Clone attempt $attempt failed, retrying in 10s ..."
      rm -rf .west-manifest-tmp
      sleep 10
    done
    if [ ! -d ".west-manifest-tmp" ]; then
      echo "ERROR: git clone failed after retries. Check network or try again later."
      exit 1
    fi
    mkdir -p zephyr
    shopt -s dotglob nullglob
    mv .west-manifest-tmp/* .west-manifest-tmp/.git zephyr/ 2>/dev/null || true
    shopt -u dotglob nullglob
    rm -rf .west-manifest-tmp
  fi
fi

# Create west workspace if .west is missing (so "west zephyr-export" works)
if [ ! -d ".west" ]; then
  echo "Creating west workspace in $ZEPHYR_DIR ..."
  west init -l zephyr
  for attempt in 1 2 3; do
    if west update; then
      break
    fi
    echo "west update attempt $attempt failed, retrying in 10s ..."
    sleep 10
  done
  if [ ! -d "zephyr/modules/hal/cmsis" ] && [ ! -d "zephyr/modules/hal/CMSIS_6" ]; then
    echo "WARNING: cmsis not found; nRF52 build may need it. Run 'west update' again if build fails."
  fi
fi

echo "Exporting Zephyr CMake package ..."
export ZEPHYR_BASE="$ZEPHYR_DIR/zephyr"
west zephyr-export

echo "Installing Zephyr Python dependencies (base) ..."
pip install -r zephyr/scripts/requirements-base.txt

echo "Installing Python dependencies (west packages) ..."
west packages pip --install

echo "Done. To use: source $ZEPHYR_DIR/.venv/bin/activate && export ZEPHYR_BASE=$ZEPHYR_DIR/zephyr"
echo "Optional: install Zephyr SDK to $ZEPHYR_DIR/sdk and set ZEPHYR_SDK_INSTALL_DIR=$ZEPHYR_DIR/sdk"
