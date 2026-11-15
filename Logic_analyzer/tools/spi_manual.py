"""Manual test of the Pico SPI analyzer (firmware commands Se/Sx/Sc/Sq/Ss/Sr).

Wiring:
    DUT SCK  (PC10) -> Pico GP10      DUT MISO (PC11) -> Pico GP12
    DUT MOSI (PC12) -> Pico GP11      DUT CS   (PD0)  -> Pico GP13      + common GND
On the HIL board JP9 these are the columns silk-screened "10", "11", "12" (bottom row) and "13" (top row).
WARNING: the DUT J3 silkscreen swaps SCK and MOSI. Wire by the PC pin names above, not by the labels.

Typical session, Pico alone on the bus (DUT SPI3 is mode 1). --reply 00 makes the Pico answer 0x00
on MISO (W25Q "not busy"), otherwise the floating MISO can make w25q_write hang the DUT:
    python spi_manual.py --mode 1 --reply 00 --expect-mosi "9F 00 00 00" --expect-miso "00 00 00 00"
        -> on the DUT console (COM7) run:   w25q_ID       (the DUT then prints ID 0x0000)
        -> press Enter here: one line per CS frame, MOSI and MISO bytes

Other DUT commands to try:  w25q_write 16   (frames 06 | 02 00 00 00 00..0F | 05 FF ...)
                            w25q_read 16    (frame 03 00 00 00 FF x16, data on MISO)
With another slave on the bus (e.g. the HIL W25Q emulator) do NOT use --reply/--device: the Pico only listens.

Register-device session (the Pico is a 16-register SPI sensor, first byte = R/W bit 7 + register):
    python spi_manual.py --device --reset --reg 00=A5 --reg 02=12 --reg 03=34
        -> on the DUT console (COM7):   spi_rd 0x00 1       -> SPI RD reg=0x00: A5
                                        spi_rd 0x02 2       -> 12 34
                                        spi_wr 0x01 0x81
        -> press Enter here: the frames (80 00 / 00 A5 ...) and the register map (register 01 = 81)
"""
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


def match(pattern, data):
    toks = pattern.split()
    return len(toks) == len(data) and all(t == "??" or int(t, 16) == b for t, b in zip(toks, data))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--port", default="COM9")
    ap.add_argument("--mode", type=int, default=1, help="SPI mode 0..3 of the DUT (default 1)")
    ap.add_argument("--expect-mosi", default=None, help='MOSI bytes of one frame, e.g. "9F 00 00 00"')
    ap.add_argument("--expect-miso", default=None, help='MISO bytes of that frame, "??" = any byte')
    ap.add_argument("--reply", choices=["00", "FF", "ff"], default=None,
                    help="Pico answers this byte on MISO (only when no other slave is connected)")
    ap.add_argument("--device", action="store_true", help="Pico acts as a 16-register SPI device")
    ap.add_argument("--reg", action="append", default=[], metavar="RR=VV",
                    help="preload a device register, hex, e.g. --reg 00=A5 (repeatable)")
    ap.add_argument("--reset", action="store_true", help="set all 16 device registers to 00 first")
    a = ap.parse_args()

    with serial.Serial(a.port, 921600, timeout=0.2) as ser:
        time.sleep(0.2)
        if a.reset:
            send(ser, "Sz")
            print("Sz ->", read_lines_until(ser, "SPI")[-1])
        for item in a.reg:
            rr, vv = item.split("=")
            send(ser, f"Sw{int(rr, 16):02X}{int(vv, 16):02X}")
            print("Sw ->", read_lines_until(ser, "SPI")[-1])
        if a.device:
            send(ser, "Sd1")
            print("Sd ->", read_lines_until(ser, "SPI")[-1])
        else:
            send(ser, "Sd0")
            read_lines_until(ser, "SPI")
            send(ser, f"Sr{a.reply.upper()}" if a.reply else "Srx")
            print("Sr ->", read_lines_until(ser, "SPI")[-1])
        send(ser, f"Se{a.mode}")
        resp = read_lines_until(ser, "SPI")[-1]
        print("Se ->", resp)
        if not resp.startswith("SPI ON"):
            print("Pico did not start the SPI analyzer (wrong firmware?)")
            return 2

        input(f"Sniffing SPI mode {a.mode}. Run the command on the DUT, then press Enter... ")

        send(ser, "Sq")
        lines = read_lines_until(ser, "END")
        if a.device:
            send(ser, "Sg")
            lines += read_lines_until(ser, "REGS")[-1:]
        send(ser, "Sx")
        read_lines_until(ser, "SPI")
        send(ser, "Sd0")   # leave the Pico passive
        read_lines_until(ser, "SPI")
        send(ser, "Srx")
        read_lines_until(ser, "SPI")

    print("--- Pico log ---")
    frames = []
    for ln in lines:
        print(ln)
        toks = ln.split()
        if "MO" in toks and "MI" in toks:
            mo, mi = toks.index("MO"), toks.index("MI")
            frames.append(([int(t, 16) for t in toks[mo + 1:mi]], [int(t, 16) for t in toks[mi + 1:-1]]))

    if a.expect_mosi is None:
        return 0
    ok = any(match(a.expect_mosi, mo) and (a.expect_miso is None or match(a.expect_miso, mi))
             for mo, mi in frames)
    print("PASS" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
