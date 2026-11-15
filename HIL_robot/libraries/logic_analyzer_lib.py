from robot.api.deco import library, keyword
from robot.api import logger
from cli_lib import CLI
import re
import time
from datetime import datetime
from robot.libraries.BuiltIn import BuiltIn
import numpy as np

LA_SAMPLE_RATE = 100000
LA_SAMPLES = 100000

@library(scope="GLOBAL")
class Logic_Library:
    """
    Robot Library for Logic Analyzer
    """

    def __init__(self):
        self.cli = CLI()

    # ------------------
    # Connection
    # ------------------
    @keyword("Connect Logic Analyzer")
    def connect_la(self, port, baud=921600):
        self.cli.connect(port, baud)

    @keyword("Disconnect Logic Analyzer")
    def disconnect_la(self):
        self.cli.disconnect()

    def setup_la(self, rate, samples):
        self.cli.setup_logic_analyzer(rate, samples)
        logger.info(f"Logic Analyzer setup: samples={samples}, rate={rate} Hz")
    
    @keyword("LA Read Data")
    def read_la_data(self):
        self.setup_la(rate=LA_SAMPLE_RATE, samples=LA_SAMPLES)  # 1 giây @ 100kHz
        data = self.cli.capture_binary_stream(timeout_sec=12.0)
        logger.info(f"Logic Analyzer data length: {len(data)} bytes")
        return data

    @keyword("LA Verify Sine Wave")
    def verify_analog_is_sine_wave(self, raw_data: bytes, expected_freq_hz=None):
        sample_rate = LA_SAMPLE_RATE
        snr_threshold = 300

        # 1. Chuyển đổi dữ liệu thô sang mảng Numpy
        arr = np.frombuffer(raw_data, dtype=np.uint8)
        
        # Bỏ byte cuối cùng nếu tổng số byte bị lẻ (đảm bảo chẵn cặp Digital/Analog)
        if len(arr) % 2 != 0:
            arr = arr[:-1]

        if len(arr) < 500:
            raise ValueError("FAIL: Dữ liệu quá ngắn (< 500 byte) để chạy phân tích FFT.")

        # 2. Tách kênh Analog (nằm ở các index lẻ: 1, 3, 5...) và giải mã điện áp 7-bit
        analog_bytes = arr[1::2]
        voltages = (analog_bytes & 0x7F) * (3.3 / 127.0)

        # 3. Tiền xử lý tín hiệu
        # Bỏ thành phần DC (dịch tâm sóng về 0V) để đồ thị FFT không bị vọt lên ở mốc 0Hz
        voltages_no_dc = voltages - np.mean(voltages)
        
        # Áp dụng hàm cửa sổ Hanning (Windowing) để làm mượt hai đầu mảng sóng, giảm rò rỉ phổ
        window = np.hanning(len(voltages_no_dc))
        voltages_windowed = voltages_no_dc * window

        # 4. Chạy thuật toán FFT và tính NĂNG LƯỢNG (Bình phương biên độ)
        fft_values = np.fft.rfft(voltages_windowed)
        fft_power = np.abs(fft_values) ** 2  # <-- FIX 1: Bình phương để lấy Power
        freqs = np.fft.rfftfreq(len(voltages_windowed), d=1.0/sample_rate)

        # 5. Phân tích năng lượng để tìm sóng Sine
        peak_idx = np.argmax(fft_power)
        peak_freq = freqs[peak_idx]

        window_size = 10  # Lấy 10 vạch bên trái và 10 vạch bên phải đỉnh chính
        start_idx = max(0, peak_idx - window_size)
        end_idx = min(len(fft_power), peak_idx + window_size + 1)

        # Tổng năng lượng của Tín hiệu chính
        peak_energy = np.sum(fft_power[start_idx:end_idx])

        # Tính tổng năng lượng của Rác (sau khi đã đào bỏ vùng tín hiệu chính)
        fft_power[start_idx:end_idx] = 0 
        noise_energy = np.sum(fft_power) + 1e-9  # Cộng 1e-9 chống lỗi chia 0

        # Tính tỷ lệ Tín hiệu trên Nhiễu (SNR) bằng năng lượng
        snr_ratio = peak_energy / noise_energy
        
        # Nếu tỷ lệ áp đảo rác => Sóng Sine
        is_sine = bool(snr_ratio >= snr_threshold)
        
        # Tính thêm V_Peak_to_Peak để báo cáo cho đẹp
        v_pp = np.max(voltages) - np.min(voltages)

        # 6. Log kết quả ra Console của Robot Framework
        logger.info(f"--- ANALOG ANALYSIS ---")
        logger.info(f"Measured Freq: {peak_freq:.2f} Hz")
        logger.info(f"Vpp: {v_pp:.2f} V")
        logger.info(f"SNR Ratio: {snr_ratio:.2f} (Threshold: {snr_threshold})")
        logger.info(f"Is Sine Wave: {is_sine}")

        if not is_sine:
            # Nếu SNR quá thấp, ném ra lỗi. Robot Framework sẽ bắt lỗi này và đánh Fail test case!
            raise AssertionError(
                f"Analog signal is not a Sine wave! "
                f"(SNR Ratio = {snr_ratio:.2f}). "
                f"Frequency of noise detected: {peak_freq:.2f} Hz."
            )

        if expected_freq_hz is not None:
            expected_freq_hz = float(expected_freq_hz) # Ép kiểu
            
            # Tính toán sai số 5%
            lower_bound = expected_freq_hz * 0.95
            upper_bound = expected_freq_hz * 1.05

            if not (lower_bound <= peak_freq <= upper_bound):
                raise AssertionError(
                    f"FAIL: Tín hiệu LÀ sóng Sine nhưng SAI tần số! "
                    f"Mong đợi: {expected_freq_hz} Hz (\u00B15%). Đo được thực tế: {peak_freq:.2f} Hz."
                )
        return is_sine

    @keyword("LA Verify Pulse Wave")
    def verify_pulse_wave(self, raw_data: bytes, channel: int, expected_freq_hz=None, expected_duty_cycle=None):
        if channel > 6:
            raise ValueError("Channel must be between 0 and 6.")

        sample_rate = LA_SAMPLE_RATE

        # 1. Ép kiểu và lọc chẵn cặp
        arr = np.frombuffer(raw_data, dtype=np.uint8)
        valid_data = arr[arr >= 0x80]
        if len(valid_data) % 2 != 0:
            valid_data = valid_data[:-1]

        if len(valid_data) < 1000:
            raise ValueError("FAIL: Không đủ dữ liệu để phân tích Digital.")

        # 2. TÁCH KÊNH DIGITAL
        digital_bytes = valid_data[0::2]
        channel_data = (digital_bytes >> int(channel)) & 1

        # 3. THUẬT TOÁN TÌM CẠNH
        diff = np.diff(channel_data)
        rising_edges = np.where(diff == 1)[0]
        
        # 4. TÍNH TOÁN THÔNG SỐ
        if len(rising_edges) < 2:
            measured_freq = 0.0
            measured_duty = 100.0 if channel_data[0] == 1 else 0.0
            logger.info(f"Signal is flat (DC). Level: {'HIGH' if measured_duty == 100 else 'LOW'}")
        else:
            first_edge = rising_edges[0]
            last_edge = rising_edges[-1]
            num_cycles = len(rising_edges) - 1
            
            time_elapsed = (last_edge - first_edge) / sample_rate
            measured_freq = num_cycles / time_elapsed
            
            exact_cycles_data = channel_data[first_edge:last_edge]
            measured_duty = np.mean(exact_cycles_data) * 100.0

        # 5. LOG KẾT QUẢ VÀO FILE ROBOT
        logger.info(f"--- DIGITAL CHANNEL D{channel} ANALYSIS ---")
        logger.info(f"Measured Freq: {measured_freq:.2f} Hz")
        logger.info(f"Measured Duty Cycle: {measured_duty:.2f} %")

        # Kiểm tra Tần số
        if expected_freq_hz is not None:
            expected_freq_hz = float(expected_freq_hz)
            lower_f = expected_freq_hz * 0.95
            upper_f = expected_freq_hz * 1.05
            if not (lower_f <= measured_freq <= upper_f):
                raise AssertionError(
                    f"FAIL (D{channel}): Sai Tần số! "
                    f"Mong đợi: {expected_freq_hz} Hz (\u00B15%). Đo được: {measured_freq:.2f} Hz."
                )

        if expected_duty_cycle is not None:
            expected_duty_cycle = float(expected_duty_cycle)
            # Tính biên độ sai số
            error_margin = expected_duty_cycle * 0.05 
            
            lower_d = expected_duty_cycle - error_margin
            upper_d = expected_duty_cycle + error_margin
            
            if not (lower_d <= measured_duty <= upper_d):
                raise AssertionError(
                    f"FAIL (D{channel}): Sai Duty Cycle! "
                    f"Mong đợi: {expected_duty_cycle}% (\u00B15% tương đối). Đo được: {measured_duty:.2f}%."
                )
        
        # Nếu code chạy được đến đây mà không bị văng AssertionError, tức là PASSED 100%
        return True

    @keyword("LA Verify Triangle Wave")
    def verify_triangle_wave(self, raw_data: bytes, expected_freq_hz=None):
        sample_rate = LA_SAMPLE_RATE

        arr = np.frombuffer(raw_data, dtype=np.uint8)

        if len(arr) % 2 != 0:
            arr = arr[:-1]

        analog_bytes = arr[1::2]
        voltages = (analog_bytes & 0x7F) * (3.3 / 127.0)

        voltages -= np.mean(voltages)
        voltages *= np.hanning(len(voltages))

        fft_vals = np.fft.rfft(voltages)
        fft_mag = np.abs(fft_vals)

        freqs = np.fft.rfftfreq(len(voltages), d=1/sample_rate)

        peak_idx = np.argmax(fft_mag[1:]) + 1
        f0 = freqs[peak_idx]

        def harmonic_energy(h):
            idx = np.argmin(np.abs(freqs - h*f0))
            return np.sum(fft_mag[max(0, idx-2):idx+3])

        A1 = harmonic_energy(1)
        A2 = harmonic_energy(2)
        A3 = harmonic_energy(3)
        A4 = harmonic_energy(4)
        A5 = harmonic_energy(5)

        logger.info(f"A1={A1:.2f}, A2={A2:.2f}, A3={A3:.2f}, A4={A4:.2f}, A5={A5:.2f}")

        # Triangle characteristics:
        # - Even harmonics small
        # - Odd harmonics decrease rapidly
        if A2 > A3 * 0.5:
            raise AssertionError("FAIL: Even harmonic too large for triangle wave.")

        if A4 > A5 * 0.5:
            raise AssertionError("FAIL: Even harmonic too large for triangle wave.")

        if not (A1 > A3 > A5):
            raise AssertionError("FAIL: Odd harmonics not decreasing properly.")

        if expected_freq_hz is not None:
            expected_freq_hz = float(expected_freq_hz)
            if not (0.95*expected_freq_hz <= f0 <= 1.05*expected_freq_hz):
                raise AssertionError(
                    f"FAIL: Wrong frequency. Expected {expected_freq_hz}, got {f0:.2f}"
                )

        logger.info(f"Triangle Wave detected. Freq={f0:.2f} Hz")
        return True

    # ------------------
    # I2C device emulation (Pico firmware with i2c_slave_emu, pins GP0=SDA, GP1=SCL)
    # The Pico is a register-based I2C device: 19 registers (0x00..0x12) behind an 8-bit pointer.
    # It ACKs its address and every byte, and logs the bus events.
    # Text commands: Ie<hh> enable, Ix disable, Ic clear, Iq dump (ends with "END"), Is,
    #                Iw<rr><vv> set register, Ig dump registers ("REGS .."), Iz reset registers
    # ------------------
    def _pico_send(self, cmd):
        ser = self.cli.ser
        if not ser:
            raise AssertionError("Logic analyzer (Pico) is not connected")
        ser.reset_input_buffer()
        ser.write((cmd + "\n").encode("ascii"))

    def _pico_read_until(self, predicate, timeout=2.0):
        """Read text lines from the Pico until predicate(line) is true. Returns all lines read."""
        ser = self.cli.ser
        lines, buf = [], b""
        end = time.time() + timeout
        while time.time() < end:
            chunk = ser.read(ser.in_waiting or 1)
            if not chunk:
                continue
            buf += chunk
            while b"\n" in buf:
                raw, buf = buf.split(b"\n", 1)
                line = raw.decode(errors="replace").strip()
                if not line:
                    continue
                lines.append(line)
                if predicate(line):
                    return lines
        raise AssertionError(f"No expected response from the Pico within {timeout}s, got: {lines}")

    @staticmethod
    def _parse_i2c_log(lines):
        """'S W 00 01 P' lines -> list of transactions: {'write': [...], 'read': [...]}.
        Address-only transactions ('S P') and stray STOPs are dropped."""
        transactions, cur, mode = [], None, None
        for line in lines:
            if line.startswith("END"):
                break
            for tok in line.split():
                if tok == "S":
                    if cur and (cur["write"] or cur["read"]):
                        transactions.append(cur)
                    cur, mode = {"write": [], "read": []}, None
                elif tok == "P":
                    if cur and (cur["write"] or cur["read"]):
                        transactions.append(cur)
                    cur, mode = None, None
                elif tok in ("W", "R"):
                    mode = "write" if tok == "W" else "read"
                elif cur is not None and mode:
                    cur[mode].append(int(tok, 16))
        if cur and (cur["write"] or cur["read"]):
            transactions.append(cur)
        return transactions

    @staticmethod
    def _to_bytes(data):
        """'00 01 0x02' / '000102' / [0, 1, 2] -> [0, 1, 2]"""
        if isinstance(data, (list, tuple)):
            return [int(str(b), 0) if str(b).lower().startswith("0x") else int(str(b), 16) for b in data]
        text = str(data).replace(",", " ").strip()
        toks = text.split()
        if len(toks) == 1 and len(toks[0]) > 2 and not toks[0].lower().startswith("0x"):
            toks = [toks[0][i:i + 2] for i in range(0, len(toks[0]), 2)]
        return [int(t, 16) for t in toks]

    @keyword("Pico I2C Slave Enable")
    def pico_i2c_slave_enable(self, addr="0x42"):
        """Make the Pico an I2C slave at 7-bit `addr` (0x08..0x77). Also clears the event log."""
        value = int(str(addr), 0)
        if not 0x08 <= value <= 0x77:
            raise AssertionError(f"I2C address out of range 0x08..0x77: {addr}")
        self._pico_send(f"Ie{value:02X}")
        resp = self._pico_read_until(lambda l: l.startswith("I2C"))[-1]
        if not resp.startswith("I2C ON"):
            raise AssertionError(f"Pico did not enable the I2C slave (firmware without I2C emulation?): {resp}")
        logger.info(f"Pico I2C slave enabled at 0x{value:02X}")

    @keyword("Pico I2C Slave Disable")
    def pico_i2c_slave_disable(self):
        self._pico_send("Ix")
        self._pico_read_until(lambda l: l.startswith("I2C"))

    @keyword("Pico I2C Slave Clear Log")
    def pico_i2c_slave_clear(self):
        self._pico_send("Ic")
        self._pico_read_until(lambda l: l.startswith("I2C"))

    I2C_REG_COUNT = 19   # 0x00..0x12 (DS3231 map size)

    def _reg_index(self, reg):
        r = int(str(reg), 0)
        if not 0 <= r < self.I2C_REG_COUNT:
            raise AssertionError(f"Register 0x{r:02X} is outside the emulated map 0x00..0x{self.I2C_REG_COUNT - 1:02X}")
        return r

    @keyword("Pico I2C Device Start")
    def pico_i2c_device_start(self, addr="0x68", reset=False):
        """Start the emulated I2C device at `addr` (also clears the event log).

        Registers keep their values (so they can be preloaded before) unless `reset=True`.
        A plain register device: the RTC mode (running time registers) is turned off.
        """
        self._pico_send("It0")
        self._pico_read_until(lambda l: l.startswith("I2C"))
        if str(reset).lower() in ("true", "1", "yes"):
            self.pico_i2c_reset_registers()
        self.pico_i2c_slave_enable(addr)

    @keyword("Pico I2C Reset Registers")
    def pico_i2c_reset_registers(self):
        """All emulated registers back to 0x00 (RTC mode: DS3231 power-on state)."""
        self._pico_send("Iz")
        self._pico_read_until(lambda l: l.startswith("I2C"))

    @keyword("Pico I2C Set Register")
    def pico_i2c_set_register(self, reg, *values):
        """Set register `reg` (and the following ones) to `values`, e.g. `0x02    0x12    0x34`."""
        r = self._reg_index(reg)
        if not values:
            raise AssertionError("Give at least one value")
        for i, v in enumerate(values):
            reg_i = self._reg_index(r + i)
            val = int(str(v), 0)
            if not 0 <= val <= 0xFF:
                raise AssertionError(f"Register value out of range: {v}")
            self._pico_send(f"Iw{reg_i:02X}{val:02X}")
            resp = self._pico_read_until(lambda l: l.startswith("I2C"))[-1]
            if not resp.startswith("I2C REG"):
                raise AssertionError(f"Pico rejected register write 0x{reg_i:02X}=0x{val:02X}: {resp}")

    @keyword("Pico I2C Get Registers")
    def pico_i2c_get_registers(self):
        """Return the whole emulated register map as a list of 19 ints."""
        self._pico_send("Ig")
        line = self._pico_read_until(lambda l: l.startswith("REGS"))[-1]
        regs = [int(t, 16) for t in line.split()[1:]]
        logger.info("Pico I2C registers: " + " ".join(f"{i:02X}:{v:02X}" for i, v in enumerate(regs)))
        return regs

    @keyword("Pico I2C Register Should Be")
    def pico_i2c_register_should_be(self, reg, *expected):
        """Register `reg` (and the following ones) must hold `expected`, e.g. `0x01    0x80`."""
        r = self._reg_index(reg)
        regs = self.pico_i2c_get_registers()
        want = [int(str(v), 0) for v in expected]
        got = regs[r:r + len(want)]
        if got != want:
            raise AssertionError(f"Pico register 0x{r:02X}: expected {self._hex(want)}, got {self._hex(got)}")

    @keyword("Pico I2C Slave Get Transactions")
    def pico_i2c_slave_get_transactions(self):
        """Read and consume the Pico log. Returns a list of {'write': [...], 'read': [...]}."""
        self._pico_send("Iq")
        lines = self._pico_read_until(lambda l: l.startswith("END"), timeout=3.0)
        logger.info("Pico I2C log:\n" + "\n".join(lines))
        end = [l for l in lines if l.startswith("END")][-1]
        if not end.endswith("DROP 0"):
            logger.warn(f"Pico I2C log overflowed: {end}")
        return self._parse_i2c_log(lines)

    @keyword("Pico I2C Should Receive")
    def pico_i2c_should_receive(self, expected, timeout=2.0):
        """Wait until the Pico has received exactly one data write and check its bytes.

        `expected` like "00 01 02 03" (hex). Address-only probes are ignored.
        """
        want = self._to_bytes(expected)
        end = time.time() + float(timeout)
        writes = []
        while True:
            writes += [t["write"] for t in self.pico_i2c_slave_get_transactions() if t["write"]]
            if writes or time.time() > end:
                break
            time.sleep(0.2)

        fmt = lambda bs: " ".join(f"{b:02X}" for b in bs)
        if not writes:
            raise AssertionError(f"Pico received no I2C data. Expected: {fmt(want)}")
        if len(writes) > 1:
            raise AssertionError("Pico received more than one I2C write: "
                                 + " | ".join(fmt(w) for w in writes) + f". Expected only: {fmt(want)}")
        if writes[0] != want:
            raise AssertionError(f"I2C data mismatch. Expected: {fmt(want)}  Received: {fmt(writes[0])}")
        logger.info(f"Pico received expected I2C data: {fmt(want)}")
        return writes[0]

    # ------------------
    # DS3231 RTC emulation (simplified) on top of the I2C device, address 0x68.
    # Registers 0x00..0x06 = sec, min, hour (24 h), day 1..7, date, month, year (BCD) and they run.
    # Firmware commands: It1 / It0 = RTC mode on / off; Iz in RTC mode = power-on state 2000-01-01 00:00:00.
    # ------------------
    RTC_ADDR = 0x68

    @keyword("Pico RTC Start")
    def pico_rtc_start(self, reset=True):
        """Start the emulated DS3231 at 0x68. `reset=True` loads the power-on time 2000-01-01 00:00:00."""
        self._pico_send("It1")
        resp = self._pico_read_until(lambda l: l.startswith("I2C"))[-1]
        if resp != "I2C RTC 1":
            raise AssertionError(f"Pico has no RTC mode (old firmware?): {resp}")
        if str(reset).lower() in ("true", "1", "yes"):
            self.pico_i2c_reset_registers()
        self.pico_i2c_slave_enable(self.RTC_ADDR)

    @keyword("Pico RTC Stop")
    def pico_rtc_stop(self):
        self._pico_send("It0")
        self._pico_read_until(lambda l: l.startswith("I2C"))
        self.pico_i2c_slave_disable()

    @staticmethod
    def _bcd(v):
        return ((v // 10) << 4) | (v % 10)

    @staticmethod
    def _unbcd(v):
        return (v >> 4) * 10 + (v & 0x0F)

    @keyword("Pico RTC Set Time")
    def pico_rtc_set_time(self, when, dow=1):
        """Set the emulated RTC, `when` = "YYYY-MM-DD HH:MM:SS" (2000..2099), `dow` = day 1..7."""
        t = datetime.strptime(str(when).strip(), "%Y-%m-%d %H:%M:%S")
        if not 2000 <= t.year <= 2099:
            raise AssertionError("Year must be 2000..2099")
        dow = int(dow)
        if not 1 <= dow <= 7:
            raise AssertionError("dow must be 1..7")
        b = self._bcd
        # registers 0x01..0x06 first, seconds last (writing the seconds restarts the 1 s countdown)
        self.pico_i2c_set_register(0x01, b(t.minute), b(t.hour), dow, b(t.day), b(t.month), b(t.year % 100))
        self.pico_i2c_set_register(0x00, b(t.second))
        logger.info(f"Pico RTC set to {t:%Y-%m-%d %H:%M:%S} (day {dow})")

    @keyword("Pico RTC Get Time")
    def pico_rtc_get_time(self):
        """Current time of the emulated RTC as "YYYY-MM-DD HH:MM:SS"."""
        r = self.pico_i2c_get_registers()
        u = self._unbcd
        year = (2100 if r[5] & 0x80 else 2000) + u(r[6])
        text = (f"{year:04d}-{u(r[5] & 0x1F):02d}-{u(r[4] & 0x3F):02d} "
                f"{u(r[2] & 0x3F):02d}:{u(r[1] & 0x7F):02d}:{u(r[0] & 0x7F):02d}")
        logger.info(f"Pico RTC time: {text} (day {r[3]})")
        return text

    @keyword("Pico RTC Time Should Be")
    def pico_rtc_time_should_be(self, expected, tolerance=1):
        """The emulated RTC must show `expected` ("YYYY-MM-DD HH:MM:SS") within `tolerance` seconds."""
        got = self.pico_rtc_get_time()
        want = datetime.strptime(str(expected).strip(), "%Y-%m-%d %H:%M:%S")
        try:
            got_t = datetime.strptime(got, "%Y-%m-%d %H:%M:%S")
        except ValueError:
            raise AssertionError(f"Pico RTC holds an invalid time: {got}")
        diff = abs((got_t - want).total_seconds())
        if diff > float(tolerance):
            raise AssertionError(f"Pico RTC time {got}, expected {expected} (+/- {tolerance} s)")

    # ------------------
    # UART receiver (Pico firmware with uart_rx_emu, RX on GP21; the Pico never drives the line)
    # Text commands: Ue<baud> enable 8N1, Ux disable, Uc clear, Uq dump (ends with "END ..."), Us
    # ------------------
    @staticmethod
    def _parse_uart_dump(lines):
        """'RX 48 65 ...' lines + 'END N 2 FE 0 PE 0 BE 0 OE 0 DROP 0' -> (bytes, stats dict)"""
        data, stats = [], {}
        for line in lines:
            toks = line.split()
            if not toks:
                continue
            if toks[0] == "RX":
                data += [int(t, 16) for t in toks[1:]]
            elif toks[0] == "END":
                stats = {toks[i]: int(toks[i + 1]) for i in range(1, len(toks) - 1, 2)}
        return data, stats

    @keyword("Pico UART Enable")
    def pico_uart_enable(self, baud=115200):
        """Start receiving 8N1 at `baud` on GP21. Also clears the log."""
        baud = int(baud)
        self._pico_send(f"Ue{baud}")
        resp = self._pico_read_until(lambda l: l.startswith("UART"))[-1]
        if not resp.startswith("UART ON"):
            raise AssertionError(f"Pico did not enable the UART receiver (firmware without UART analyzer?): {resp}")
        logger.info(f"Pico UART receiver enabled at {baud} baud")

    @keyword("Pico UART Disable")
    def pico_uart_disable(self):
        self._pico_send("Ux")
        self._pico_read_until(lambda l: l.startswith("UART"))

    @keyword("Pico UART Clear")
    def pico_uart_clear(self):
        self._pico_send("Uc")
        self._pico_read_until(lambda l: l.startswith("UART"))

    @keyword("Pico UART Get Data")
    def pico_uart_get_data(self, settle=0.3, timeout=3.0):
        """Read and consume everything received so far.

        Waits until no new byte arrives for `settle` seconds (the DUT may still be sending).
        Returns a dict: {'data': [bytes...], 'FE': n, 'PE': n, 'BE': n, 'OE': n, 'DROP': n}
        """
        data, totals = [], {"FE": 0, "PE": 0, "BE": 0, "OE": 0, "DROP": 0}
        end = time.time() + float(timeout)
        while True:
            self._pico_send("Uq")
            lines = self._pico_read_until(lambda l: l.startswith("END"), timeout=3.0)
            chunk, stats = self._parse_uart_dump(lines)
            data += chunk
            for k in totals:
                totals[k] += stats.get(k, 0)
            if (not chunk and data) or time.time() > end:
                break
            time.sleep(float(settle))
        totals["data"] = data
        logger.info(f"Pico UART: {len(data)} bytes, errors FE={totals['FE']} PE={totals['PE']} "
                    f"BE={totals['BE']} OE={totals['OE']} DROP={totals['DROP']}")
        logger.info("Pico UART hex: " + " ".join(f"{b:02X}" for b in data))
        return totals

    def _check_uart_errors(self, rx):
        errs = {k: rx[k] for k in ("FE", "PE", "BE", "OE", "DROP") if rx[k]}
        if errs:
            raise AssertionError(f"UART line errors {errs} (FE/BE usually mean a wrong baud rate)")

    @keyword("Pico UART Should Receive Bytes")
    def pico_uart_should_receive_bytes(self, expected, timeout=3.0):
        """Exact byte check, `expected` like "00 01 02" (hex). Also fails on any line error."""
        want = self._to_bytes(expected)
        rx = self.pico_uart_get_data(timeout=timeout)
        self._check_uart_errors(rx)
        got = rx["data"]
        if got != want:
            first = next((i for i, (a, b) in enumerate(zip(got, want)) if a != b), min(len(got), len(want)))
            raise AssertionError(f"UART data mismatch: expected {len(want)} bytes, got {len(got)}; "
                                 f"first difference at index {first}")
        return got

    @keyword("Pico UART Should Receive Text")
    def pico_uart_should_receive_text(self, expected, timeout=3.0):
        """Text check; leading/trailing whitespace and CR/LF are ignored. Also fails on any line error."""
        rx = self.pico_uart_get_data(timeout=timeout)
        self._check_uart_errors(rx)
        text = bytes(rx["data"]).decode("ascii", errors="replace").strip()
        if text != str(expected).strip():
            raise AssertionError(f"UART text mismatch\nExpected: '{str(expected).strip()}'\nReceived: '{text}'")
        return text

    # ------------------
    # Passive SPI analyzer (Pico firmware with spi_sniff: GP10 SCK, GP11 MOSI, GP12 MISO, GP13 CS)
    # The Pico only listens, so the real slave (e.g. the HIL W25Q emulator) stays on the bus.
    # Text commands: Se<mode> start, Sx stop, Sc clear, Sq dump (ends with "END ..."), Ss
    # ------------------
    @staticmethod
    def _parse_spi_dump(lines):
        """'S MO 9F 00 MI FF EF P' lines -> list of
        {'mosi': [...], 'miso': [...], 'has_start': bool, 'has_stop': bool, 'complete': bool}.
        Lines without any data byte (stray CS edges such as '- MO MI P') are dropped."""
        frames, drop = [], 0
        for line in lines:
            toks = line.split()
            if not toks:
                continue
            if toks[0] == "END":
                drop = int(toks[toks.index("DROP") + 1]) if "DROP" in toks else 0
                continue
            if "MO" not in toks or "MI" not in toks:
                continue
            mo, mi = toks.index("MO"), toks.index("MI")
            mosi = [int(t, 16) for t in toks[mo + 1:mi]]
            miso = [int(t, 16) for t in toks[mi + 1:-1]]
            if not mosi and not miso:
                continue
            frames.append({
                "mosi": mosi, "miso": miso,
                "has_start": toks[0] == "S", "has_stop": toks[-1] == "P",
                "complete": toks[0] == "S" and toks[-1] == "P",
            })
        return frames, drop

    @staticmethod
    def _match_pattern(pattern, data):
        """`pattern` hex string, '??' matches any byte. Lengths must be equal."""
        toks = str(pattern).replace(",", " ").split()
        if len(toks) != len(data):
            return False
        return all(t == "??" or int(t, 16) == b for t, b in zip(toks, data))

    @staticmethod
    def _hex(data):
        return " ".join(f"{b:02X}" for b in data)

    @keyword("Pico SPI Enable")
    def pico_spi_enable(self, mode=0):
        """Start sniffing SPI `mode` 0..3 (CPOL/CPHA of the DUT). Also clears the log."""
        mode = int(mode)
        self._pico_send(f"Se{mode}")
        resp = self._pico_read_until(lambda l: l.startswith("SPI"))[-1]
        if not resp.startswith("SPI ON"):
            raise AssertionError(f"Pico did not start the SPI analyzer (firmware without SPI sniffer?): {resp}")
        logger.info(f"Pico SPI analyzer started, mode {mode}")

    @keyword("Pico SPI Disable")
    def pico_spi_disable(self):
        self._pico_send("Sx")
        self._pico_read_until(lambda l: l.startswith("SPI"))

    @keyword("Pico SPI Clear")
    def pico_spi_clear(self):
        self._pico_send("Sc")
        self._pico_read_until(lambda l: l.startswith("SPI"))

    @keyword("Pico SPI Slave Reply")
    def pico_spi_slave_reply(self, value="0x00"):
        """Make the Pico answer as the SPI slave: every MISO byte reads `value` (0x00 or 0xFF) while CS is low.

        `value=off` makes the Pico passive again. Only use 0x00/0xFF when NO other slave is on the bus.
        The setting is kept across `Pico SPI Enable` / `Pico SPI Disable`.
        """
        v = str(value).strip().lower()
        if v == "off":
            cmd = "Srx"
        elif v in ("0x00", "00", "0"):
            cmd = "Sr00"
        elif v in ("0xff", "ff", "255"):
            cmd = "SrFF"
        else:
            raise AssertionError(f"Pico SPI reply must be 0x00, 0xFF or off, not {value}")
        self._pico_send(cmd)
        resp = self._pico_read_until(lambda l: l.startswith("SPI"))[-1]
        if not resp.startswith("SPI RPL"):
            raise AssertionError(f"Pico did not accept the SPI reply setting: {resp}")
        logger.info(f"Pico SPI reply: {resp}")

    # ---- SPI register device: the Pico is a small SPI sensor (16 registers, modes 1/3) ----
    # Frame: first byte = R/W bit (bit 7, 1 = read) + register address, then data (auto-increment).
    SPI_REG_COUNT = 16

    def _spi_reg_index(self, reg):
        r = int(str(reg), 0)
        if not 0 <= r < self.SPI_REG_COUNT:
            raise AssertionError(f"Register 0x{r:02X} is outside the emulated map 0x00..0x{self.SPI_REG_COUNT - 1:02X}")
        return r

    @keyword("Pico SPI Device Start")
    def pico_spi_device_start(self, mode=1, reset=False):
        """Start the Pico as a register-based SPI device (SPI mode 1 or 3). Also clears the bus log.

        Registers keep their values (so they can be preloaded before) unless `reset=True`.
        Only use it when no other slave drives MISO.
        """
        if str(reset).lower() in ("true", "1", "yes"):
            self.pico_spi_reset_registers()
        self._pico_send("Sd1")
        resp = self._pico_read_until(lambda l: l.startswith("SPI"))[-1]
        if not resp.startswith("SPI DEV ON"):
            raise AssertionError(f"Pico did not enter SPI device mode (firmware too old?): {resp}")
        self.pico_spi_enable(mode)

    @keyword("Pico SPI Device Stop")
    def pico_spi_device_stop(self):
        """Leave device mode and stop the SPI analyzer (MISO released)."""
        self._pico_send("Sd0")
        self._pico_read_until(lambda l: l.startswith("SPI"))
        self.pico_spi_disable()

    @keyword("Pico SPI Reset Registers")
    def pico_spi_reset_registers(self):
        self._pico_send("Sz")
        self._pico_read_until(lambda l: l.startswith("SPI"))

    @keyword("Pico SPI Set Register")
    def pico_spi_set_register(self, reg, *values):
        """Set register `reg` (and the following ones) to `values`, e.g. `0x02    0x12    0x34`."""
        r = self._spi_reg_index(reg)
        if not values:
            raise AssertionError("Give at least one value")
        for i, v in enumerate(values):
            reg_i = self._spi_reg_index(r + i)
            val = int(str(v), 0)
            if not 0 <= val <= 0xFF:
                raise AssertionError(f"Register value out of range: {v}")
            self._pico_send(f"Sw{reg_i:02X}{val:02X}")
            resp = self._pico_read_until(lambda l: l.startswith("SPI"))[-1]
            if not resp.startswith("SPI REG"):
                raise AssertionError(f"Pico rejected register write 0x{reg_i:02X}=0x{val:02X}: {resp}")

    @keyword("Pico SPI Get Registers")
    def pico_spi_get_registers(self):
        """Return the whole emulated SPI register map as a list of 16 ints."""
        self._pico_send("Sg")
        line = self._pico_read_until(lambda l: l.startswith("REGS"))[-1]
        regs = [int(t, 16) for t in line.split()[1:]]
        logger.info("Pico SPI registers: " + " ".join(f"{i:02X}:{v:02X}" for i, v in enumerate(regs)))
        return regs

    @keyword("Pico SPI Register Should Be")
    def pico_spi_register_should_be(self, reg, *expected):
        """Register `reg` (and the following ones) must hold `expected`."""
        r = self._spi_reg_index(reg)
        regs = self.pico_spi_get_registers()
        want = [int(str(v), 0) for v in expected]
        got = regs[r:r + len(want)]
        if got != want:
            raise AssertionError(f"Pico SPI register 0x{r:02X}: expected {self._hex(want)}, got {self._hex(got)}")

    @keyword("Pico SPI Get Frames")
    def pico_spi_get_frames(self, settle=0.3, timeout=3.0):
        """Read and consume the captured CS frames. Waits until no new frame arrives for `settle` s.

        Returns a list of {'mosi': [...], 'miso': [...], 'complete': bool}.
        """
        frames, dropped = [], 0
        end = time.time() + float(timeout)
        while True:
            self._pico_send("Sq")
            lines = self._pico_read_until(lambda l: l.startswith("END"), timeout=3.0)
            chunk, drop = self._parse_spi_dump(lines)
            dropped += drop
            # a frame still in progress is split between two dumps: glue it back together
            if frames and chunk and not frames[-1]["has_stop"] and not chunk[0]["has_start"]:
                last = frames[-1]
                last["mosi"] += chunk[0]["mosi"]
                last["miso"] += chunk[0]["miso"]
                last["has_stop"] = chunk[0]["has_stop"]
                last["complete"] = last["has_start"] and last["has_stop"]
                chunk = chunk[1:]
            frames += chunk
            if (not chunk and frames) or time.time() > end:
                break
            time.sleep(float(settle))
        if dropped:
            raise AssertionError(f"Pico SPI log overflowed ({dropped} events dropped)")
        logger.info(f"Pico SPI: {len(frames)} frame(s)\n" + "\n".join(
            f"MOSI {self._hex(f['mosi'])} | MISO {self._hex(f['miso'])}{'' if f['complete'] else '  (incomplete)'}"
            for f in frames))
        return frames

    @keyword("Pico SPI Should Contain Frame")
    def pico_spi_should_contain_frame(self, mosi, miso=None, timeout=3.0):
        """Pass if one captured CS frame has exactly this MOSI (and MISO, if given). '??' = any byte.

        Returns the matching frame so the test can inspect its bytes.
        """
        frames = self.pico_spi_get_frames(timeout=timeout)
        for f in frames:
            if f["complete"] and self._match_pattern(mosi, f["mosi"]) \
                    and (miso is None or self._match_pattern(miso, f["miso"])):
                return f
        raise AssertionError(f"No SPI frame with MOSI [{mosi}]" + (f" / MISO [{miso}]" if miso else "")
                             + f". Captured: " + " | ".join(self._hex(f["mosi"]) for f in frames))

    @keyword("Pico SPI Should Receive Frames")
    def pico_spi_should_receive_frames(self, *mosi_frames, then_only=None, timeout=3.0):
        """The first captured frames must have these MOSI bytes, in this order ('??' = any byte).

        Any further frame must match `then_only` (e.g. "05 FF" = W25Q status polling), or none is allowed.
        """
        frames = self.pico_spi_get_frames(timeout=timeout)
        if len(frames) < len(mosi_frames):
            raise AssertionError(f"Expected at least {len(mosi_frames)} SPI frames, captured {len(frames)}: "
                                 + " | ".join(self._hex(f["mosi"]) for f in frames))
        for i, (want, f) in enumerate(zip(mosi_frames, frames)):
            if not f["complete"] or not self._match_pattern(want, f["mosi"]):
                raise AssertionError(f"SPI frame {i}: expected MOSI [{want}], got [{self._hex(f['mosi'])}]"
                                     + ("" if f["complete"] else " (incomplete)"))
        for i, f in enumerate(frames[len(mosi_frames):], start=len(mosi_frames)):
            if then_only is None or not self._match_pattern(then_only, f["mosi"]):
                raise AssertionError(f"Unexpected extra SPI frame {i}: MOSI [{self._hex(f['mosi'])}]")
        return frames
