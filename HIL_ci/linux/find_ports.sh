#!/usr/bin/env bash
# List the USB serial ports with their stable /dev/serial/by-id/ names, USB IDs and serial numbers,
# to fill HIL_PORT / HIL_DUT_PORT / HIL_LOG_PORT in ~/.config/hil-ci/hil-ci.env.
#
# The DUT port only exists while the DUT is powered (the HIL board switches its supply).
# To tell the HIL board and the DUT apart (both are STMicroelectronics 0483:xxxx), run this
# script, unplug/replug one board, and run it again.
set -u
shopt -s nullglob
links=(/dev/serial/by-id/*)
if [ ${#links[@]} -eq 0 ]; then
    echo "No USB serial device found (/dev/serial/by-id/ is empty)."
    exit 1
fi
for link in "${links[@]}"; do
    dev=$(readlink -f "$link")
    props=$(udevadm info -q property -n "$dev" 2>/dev/null)
    get() { printf '%s\n' "$props" | sed -n "s/^$1=//p"; }
    echo "$link"
    echo "    -> $dev   USB $(get ID_VENDOR_ID):$(get ID_MODEL_ID)   $(get ID_VENDOR) / $(get ID_MODEL)   serial $(get ID_SERIAL_SHORT)"
done
echo
echo "Hint: 2e8a = Raspberry Pi (Pico 2 -> HIL_LOG_PORT); 0483 = STMicroelectronics (HIL board or DUT)."
