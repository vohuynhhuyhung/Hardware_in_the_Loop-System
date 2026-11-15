"""Manual test of the Pico I2C device emulation (firmware commands Ie/Ix/Ic/Iq/Is/Iw/Ig/Iz).

Wiring:  DUT SDA -> Pico GP0,  DUT SCL -> Pico GP1,  common GND (DUT bus already has pull-ups).

The Pico is a register-based device: 16 registers (0x00..0x0F) behind an 8-bit pointer.
Register-device session (preload WHO_AM_I = 0x5A at register 0x00):
    python i2c_slave_manual.py --addr 68 --reset --reg 00=5A --reg 02=12 --reg 03=34
        -> on the DUT console (COM7):   i2c_rd 0x68 0x00 1        -> I2C RD ...: 5A
                                        i2c_rd 0x68 0x02 2        -> 12 34
                                        i2c_wr 0x68 0x01 0x81
        -> press Enter here: the bus log and the register map (register 01 now 81) are printed

Raw-transfer session (default address 0x42 = I2C_TEST_ADDR of the DUT firmware):
    python i2c_slave_manual.py --port COM9 --expect "00 01 02 03 04 05 06 07"
        -> the Pico now ACKs address 0x42 and every data byte written to it
        -> on the DUT console (COM7) run:   i2c_tx 8
        -> press Enter here to dump what the Pico received:   S W 00 01 02 03 04 05 06 07 P

i2c_tx <len> [start] sends bytes start, start+1, ... (len 1..64) to 0x42.
Address-only transactions ("S P") and stray STOPs are ignored by --expect.
"""
import argparse
import sys
import time

import serial


def send(ser, text):
    ser.reset_input_buffer()
    ser.write((text + "\n").encode("ascii"))


def read_line(ser, timeout=2.0):
    end = time.time() + timeout
    buf = b""
    while time.time() < end:
        c = ser.read(1)
        if not c:
            continue
        buf += c
        if c == b"\n":
            return buf.decode(errors="replace").strip()
    return buf.decode(errors="replace").strip()


def dump(ser, timeout=5.0):
    """Send Iq and return the list of lines up to the END marker."""
    send(ser, "Iq")
    lines, end = [], time.time() + timeout
    while time.time() < end:
        line = read_line(ser, 1.0)
        if not line:
            continue
        if line.startswith("END"):
            lines.append(line)
            return lines
        lines.append(line)
    raise TimeoutError("No END marker from the Pico (is it running the I2C-emulation firmware?)")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--port", default="COM9", help="Pico CDC port (default COM9)")
    ap.add_argument("--addr", default="42", help="7-bit slave address in hex (default 42)")
    ap.add_argument("--reg", action="append", default=[], metavar="RR=VV",
                    help="preload a register before starting, hex, e.g. --reg 00=5A (repeatable)")
    ap.add_argument("--reset", action="store_true", help="set all 16 registers to 00 first")
    ap.add_argument("--expect", default=None,
                    help='expected master-write bytes of the first transaction, e.g. "00 00 00 01 02"')
    ap.add_argument("--no-wait", action="store_true", help="dump immediately, do not wait for Enter")
    a = ap.parse_args()

    with serial.Serial(a.port, 921600, timeout=0.2) as ser:
        time.sleep(0.2)
        if a.reset:
            send(ser, "Iz")
            print("Iz ->", read_line(ser))
        for item in a.reg:
            rr, vv = item.split("=")
            send(ser, f"Iw{int(rr, 16):02X}{int(vv, 16):02X}")
            print("Iw ->", read_line(ser))
        send(ser, f"Ie{a.addr}")
        resp = read_line(ser)
        print("Ie ->", resp)
        if "ON" not in resp:
            print("Pico did not enable the slave (wrong firmware or invalid address 08..77)")
            return 2

        if not a.no_wait:
            input(f"Slave 0x{int(a.addr, 16):02X} is listening. Trigger the transfer on the DUT, then press Enter... ")

        send(ser, "Is")
        print("Is ->", read_line(ser))
        lines = dump(ser)
        print("--- Pico log ---")
        for ln in lines:
            print(ln)
        send(ser, "Ig")
        print(read_line(ser))   # REGS r00 r01 ... r0F

        send(ser, "Ix")
        read_line(ser)

    if a.expect is not None:
        want = [b.upper() for b in a.expect.split()]
        got = []
        for ln in lines:
            toks = ln.split()
            if "S" in toks and "W" in toks:
                # first transaction: bytes after "W" until the next marker
                i = toks.index("W") + 1
                while i < len(toks) and toks[i] not in ("S", "P", "R", "W"):
                    got.append(toks[i].upper())
                    i += 1
                break
        ok = got == want
        print(f"expected: {' '.join(want)}\nreceived: {' '.join(got)}\n{'PASS' if ok else 'FAIL'}")
        return 0 if ok else 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
