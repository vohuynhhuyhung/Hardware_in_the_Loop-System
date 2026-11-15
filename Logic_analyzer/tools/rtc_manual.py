"""Manual test of the Pico DS3231 RTC emulation (I2C 0x68, firmware commands Ie/It/Iz/Iw/Ig).

Wiring (same as the I2C device):  DUT SDA -> Pico GP0,  DUT SCL -> Pico GP1,  common GND.
No other device at 0x68 on the bus (the HIL RTC emulation must be off).

Simplified DS3231: registers 0x00..0x06 = sec, min, hour (24 h), day 1..7, date, month, year (BCD)
and they advance every second (month lengths, leap years). The other registers are plain storage.

Typical session:
    python rtc_manual.py --set "2026-12-31 23:59:50" --dow 4 --watch 15
        -> the Pico time runs across midnight / new year: 2027-01-01 00:00:05 ...
    python rtc_manual.py --set "2026-11-13 10:20:30"
        -> on the DUT console (COM7):  rtc_init
                                       rtc_get_time        -> 10:20:3x
                                       rtc_set_time 8 15 0
        -> python rtc_manual.py --watch 3      shows 2026-11-13 08:15:0x (the DUT changed the Pico time)
    python rtc_manual.py --stop     turn the RTC off (the Pico stops answering at 0x68)
The script leaves the RTC running when it exits, so the DUT can keep talking to it.
"""
import argparse
import sys
import time
from datetime import datetime

import serial


def send(ser, text):
    ser.reset_input_buffer()
    ser.write((text + "\n").encode("ascii"))


def read_line(ser, prefix, timeout=2.0):
    buf, end = b"", time.time() + timeout
    while time.time() < end:
        chunk = ser.read(ser.in_waiting or 1)
        if not chunk:
            continue
        buf += chunk
        while b"\n" in buf:
            raw, buf = buf.split(b"\n", 1)
            line = raw.decode(errors="replace").strip()
            if line.startswith(prefix):
                return line
    raise TimeoutError(f"No '{prefix}' line from the Pico")


def cmd(ser, text, prefix="I2C"):
    send(ser, text)
    return read_line(ser, prefix)


def bcd(v):
    return ((v // 10) << 4) | (v % 10)


def unbcd(v):
    return (v >> 4) * 10 + (v & 0x0F)


def get_time(ser):
    regs = [int(t, 16) for t in cmd(ser, "Ig", "REGS").split()[1:]]
    year = (2100 if regs[5] & 0x80 else 2000) + unbcd(regs[6])
    return (f"{year:04d}-{unbcd(regs[5] & 0x1F):02d}-{unbcd(regs[4] & 0x3F):02d} "
            f"{unbcd(regs[2] & 0x3F):02d}:{unbcd(regs[1] & 0x7F):02d}:{unbcd(regs[0] & 0x7F):02d}"
            f"  day {regs[3]}"), regs


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--port", default="COM9")
    ap.add_argument("--set", default=None, metavar='"YYYY-MM-DD HH:MM:SS"', help="set the RTC time")
    ap.add_argument("--dow", type=int, default=1, help="day of week 1..7 used with --set (default 1)")
    ap.add_argument("--reset", action="store_true", help="power-on state 2000-01-01 00:00:00")
    ap.add_argument("--watch", type=int, default=0, metavar="N", help="print the time every second, N times")
    ap.add_argument("--stop", action="store_true", help="turn the RTC off")
    a = ap.parse_args()

    with serial.Serial(a.port, 921600, timeout=0.2) as ser:
        time.sleep(0.2)
        if a.stop:
            print("It0 ->", cmd(ser, "It0"))
            print("Ix  ->", cmd(ser, "Ix"))
            return 0

        resp = cmd(ser, "It1")
        print("It1 ->", resp)
        if resp != "I2C RTC 1":
            print("Pico has no RTC mode (flash the new firmware)")
            return 2
        if a.reset:
            print("Iz  ->", cmd(ser, "Iz"))
        if a.set:
            t = datetime.strptime(a.set, "%Y-%m-%d %H:%M:%S")
            values = {1: bcd(t.minute), 2: bcd(t.hour), 3: a.dow, 4: bcd(t.day),
                      5: bcd(t.month), 6: bcd(t.year % 100), 0: bcd(t.second)}   # seconds last
            for reg, val in values.items():
                cmd(ser, f"Iw{reg:02X}{val:02X}")
            print(f"set {a.set} day {a.dow}")
        print("Ie68 ->", cmd(ser, "Ie68"))

        text, regs = get_time(ser)
        print("REGS", " ".join(f"{v:02X}" for v in regs))
        print("time", text)
        for _ in range(a.watch):
            time.sleep(1.0)
            print("time", get_time(ser)[0])
    return 0


if __name__ == "__main__":
    sys.exit(main())
