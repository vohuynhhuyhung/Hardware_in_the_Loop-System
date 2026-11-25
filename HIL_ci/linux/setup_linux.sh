#!/usr/bin/env bash
# One-time setup of a Linux PC (Debian/Ubuntu) as the HIL bench.
#
#   bash HIL_ci/linux/setup_linux.sh                    # packages, venv, udev, groups, env file, service file
#   bash HIL_ci/linux/setup_linux.sh --enable-service   # also start the watcher now and at every boot
#
# Run it as the user that will own the bench (not root); it calls sudo where needed.
# It can be run again safely: an existing env file is never overwritten.
# STM32CubeProgrammer and STM32CubeIDE are NOT installed here (ST asks for a login to
# download them): see HIL_ci/linux/README.md.
set -euo pipefail

ENABLE_SERVICE=0
[ "${1:-}" = "--enable-service" ] && ENABLE_SERVICE=1

if [ "$(id -u)" -eq 0 ]; then
    echo "Run this as the bench user, not as root (it uses sudo itself)." >&2
    exit 1
fi

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HIL_ROOT="$(cd "$HERE/../.." && pwd)"
BENCH_USER="$(id -un)"
ENV_DIR="$HOME/.config/hil-ci"
ENV_FILE="$ENV_DIR/hil-ci.env"
VENV="$HOME/hil-venv"
SERVICE=/etc/systemd/system/hil-watcher.service

step() { printf '\n==> %s\n' "$*"; }

step "1/6 System packages"
sudo apt-get update -qq
# libusb: STM32_Programmer_CLI talks to the DFU bootloader through it
sudo apt-get install -y git python3 python3-venv python3-pip libusb-1.0-0 udev

step "2/6 Python venv $VENV (robotframework, pyserial, numpy)"
[ -d "$VENV" ] || python3 -m venv "$VENV"
"$VENV/bin/python" -m pip install -q --upgrade pip
"$VENV/bin/python" -m pip install -q -r "$HIL_ROOT/HIL_ci/requirements.txt"
"$VENV/bin/python" -m robot --version || true

step "3/6 Serial port and USB access for $BENCH_USER"
sudo usermod -aG dialout,plugdev "$BENCH_USER"
sudo install -m 0644 "$HERE/99-hil-bench.rules" /etc/udev/rules.d/99-hil-bench.rules
sudo udevadm control --reload-rules
sudo udevadm trigger

step "4/6 Bench settings $ENV_FILE"
mkdir -p "$ENV_DIR"
if [ -f "$ENV_FILE" ]; then
    echo "Kept the existing $ENV_FILE"
else
    sed "s#@HOME@#$HOME#g" "$HERE/hil-ci.env.example" > "$ENV_FILE"
    echo "Created $ENV_FILE: fill in the three serial ports (run HIL_ci/linux/find_ports.sh)"
fi
mkdir -p "$HOME/hil_ci_data"

step "5/6 Watcher service $SERVICE"
sudo tee "$SERVICE" > /dev/null <<EOF
[Unit]
Description=HIL CI watcher (tests new commits of the registered repos on this bench)
After=network-online.target
Wants=network-online.target

[Service]
User=$BENCH_USER
WorkingDirectory=$HIL_ROOT
EnvironmentFile=$ENV_FILE
ExecStart=$VENV/bin/python $HIL_ROOT/HIL_ci/watch_registry.py
Restart=always
RestartSec=30

[Install]
WantedBy=multi-user.target
EOF
sudo systemctl daemon-reload

step "6/6 Checks"
chmod +x "$HERE/find_ports.sh"
grep -q CHANGE_ME "$ENV_FILE" && echo "!! Serial ports in $ENV_FILE are not set yet"
. <(sed -n 's/^\(HIL_[A-Z0-9_]*\)=\(.*\)$/\1="\2"/p' "$ENV_FILE")
[ -x "${HIL_STM32_PROGRAMMER:-}" ] && echo "OK  STM32_Programmer_CLI: $HIL_STM32_PROGRAMMER" \
    || echo "!! STM32_Programmer_CLI not found at ${HIL_STM32_PROGRAMMER:-?} (install STM32CubeProgrammer)"
[ -x "${HIL_CUBEIDE:-}" ] && echo "OK  STM32CubeIDE: $HIL_CUBEIDE" \
    || echo "--  STM32CubeIDE not found at ${HIL_CUBEIDE:-?} (only needed for repos without a prebuilt hex)"
if systemctl is-active --quiet ModemManager; then
    echo "--  ModemManager is running: the udev rule makes it ignore the bench ports."
    echo "    If this PC has no modem, it is simpler to disable it: sudo systemctl disable --now ModemManager"
fi

if [ "$ENABLE_SERVICE" -eq 1 ]; then
    sudo systemctl enable --now hil-watcher.service
    systemctl --no-pager status hil-watcher.service | head -5
else
    echo
    echo "Service installed but not started. When the ports are set and a manual run passes:"
    echo "    sudo systemctl enable --now hil-watcher.service"
fi
echo
echo "Log out and back in (or reboot) so the dialout/plugdev group membership takes effect."
