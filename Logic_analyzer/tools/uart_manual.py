"""Manual test of the Pico UART receiver (firmware commands Ue/Ux/Uc/Uq/Us).

Wiring:  DUT TX (USART1, PA9) -> Pico GP21,  common GND.  The Pico only listens.
         On the HIL board GP21 is JP9 pin 27 (top-row column silk-screened "21"),
         not JP9 pin 30 (GP27, an ADC pin that cannot receive UART).

Typical session:
    python uart_manual.py --port COM9 --baud 115200 --expect-text "Hello Pico"
        -> on the DUT console (COM7) run:   uart_tx Hello Pico
        -> press Enter here: the received bytes, text and line-error counters are printed

    uart_dump on the DUT sends the 256 bytes 00..FF: use --expect-count 256.
    A wrong --baud shows up as framing errors (FE) and garbage text.
"""
import os
import argparse
import sys
import time

import serial


def send(ser, text):
    ser.reset_input_buffer()
    ser.write((text + "\n").encode("ascii"))


def read_lines_until(ser, prefix, timeout=3.0):
    lines, buf, end = [], b"", time.time() + timeout
    while time.time() < end:
        chunk = ser.read(ser.in_waiting or 1)
        if not chunk:
            continue
        buf += chunk
        while b"\n" in buf:
            raw, buf = buf.split(b"\n", 1)
            line = raw.decode(errors="replace").strip()
            if line:
                lines.append(line)
                if line.startswith(prefix):
                    return lines
    raise TimeoutError(f"No '{prefix}' line from the Pico, got: {lines}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--port", default=os.environ.get("HIL_LOG_PORT", "COM9"), help="Pico CDC port (default: env HIL_LOG_PORT or COM9)")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--expect-text", default=None, help="expected text (whitespace/CRLF trimmed)")
    ap.add_argument("--expect-count", type=int, default=None, help="expected number of bytes")
    a = ap.parse_args()

    with serial.Serial(a.port, 921600, timeout=0.2) as ser:
        time.sleep(0.2)
        send(ser, f"Ue{a.baud}")
        resp = read_lines_until(ser, "UART")[-1]
        print("Ue ->", resp)
        if not resp.startswith("UART ON"):
            print("Pico did not enable the UART receiver (wrong firmware?)")
            return 2

        input(f"Listening on GP21 at {a.baud} baud. Send from the DUT, then press Enter... ")

        send(ser, "Uq")
        lines = read_lines_until(ser, "END")
        send(ser, "Ux")
        read_lines_until(ser, "UART")

    data = []
    for ln in lines:
        if ln.startswith("RX"):
            data += [int(t, 16) for t in ln.split()[1:]]
    print("--- Pico log ---")
    for ln in lines:
        print(ln)
    text = bytes(data).decode("ascii", errors="replace")
    print(f"text: {text!r}")

    end = lines[-1].split()
    stats = {end[i]: int(end[i + 1]) for i in range(1, len(end) - 1, 2)}
    errors = {k: v for k, v in stats.items() if k in ("FE", "PE", "BE", "OE", "DROP") and v}

    ok = not errors
    if errors:
        print(f"line errors: {errors}")
    if a.expect_text is not None:
        ok &= text.strip() == a.expect_text.strip()
    if a.expect_count is not None:
        ok &= len(data) == a.expect_count
    if a.expect_text is not None or a.expect_count is not None:
        print("PASS" if ok else "FAIL")
        return 0 if ok else 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
