from robot.api.deco import library, keyword
from robot.api import logger
from cli_lib import CLI
import re
import time

@library(scope="GLOBAL")
class DUTLibrary:
    """
    Robot Library for DUT firmware
    """

    def __init__(self):
        self.cli = CLI()

    # ------------------
    # Internal CLI
    # ------------------
    def _dut_cmd(self, cmd, expect_response=True):
        resp = self.cli.execute(cmd)
        resp = resp.replace("\r", "").strip()
        resp = resp.replace("DUT:~", "").strip() # Xóa prefix "DUT:~" nếu có

        logger.info(f"{resp}")

        if expect_response and not resp.strip():
            raise AssertionError(f"No response for command: {cmd}")

        return resp

    # ------------------
    # Connection
    # ------------------
    @keyword("Connect DUT")
    def connect_dut(self, port, baud):
        self.cli.connect(port, baud)

    @keyword("Disconnect DUT")
    def disconnect_dut(self):
        self.cli.disconnect()

    @keyword("DUT Help")
    def dut_help(self):

        resp = self._dut_cmd("help")

        return resp

    @keyword("Verify DUT Alive")
    def verify_dut_alive(self):

        resp = self._dut_cmd("help")

        if "help" not in resp.lower():
            raise AssertionError("Invalid DUT response")

        return resp

    # ------------------
    # PWM
    # ------------------
    @keyword("DUT Start PWM")
    def start_pwm(self, ch):
        self._dut_cmd(f"pwm_start {ch}", expect_response=False)

    @keyword("DUT Stop PWM")
    def stop_pwm(self, ch):
        self._dut_cmd(f"pwm_stop {ch}", expect_response=False)

    @keyword("DUT Set duty cycle")
    def set_pwm_duty(self, ch, duty):
        self._dut_cmd(f"pwm_set_duty_cycle {ch} {duty}", expect_response=False)

    @keyword("DUT Set PWM freq")
    def set_pwm_freq(self, freq):
        self._dut_cmd(f"pwm_set_freq {freq}", expect_response=False)

    # ------------------
    # I2C
    # ------------------

    def _validate_i2c_addr(self, addr):

        try:
            value = int(str(addr), 0)
        except ValueError:
            raise AssertionError(f"Invalid I2C address: {addr}")

        if value <= 0 or value >= 0x7F:
            raise AssertionError(
                f"I2C address out of range: {addr}"
            )

        return value


    def _validate_uint(self, name, val):

        try:
            v = int(val)
        except ValueError:
            raise AssertionError(f"{name} must be integer")

        if v < 0:
            raise AssertionError(f"{name} must be a positive integer")

        return v


    # ------------------
    # EEPROM 
    # ------------------

    @keyword("DUT Init EEPROM")
    def dut_init_eeprom(self, addr, size, page):

        addr = self._validate_i2c_addr(addr)
        size = self._validate_uint("size", size)
        page = self._validate_uint("page", page)

        if size % page != 0:
            raise AssertionError(
                "EEPROM size must be multiple of page size"
            )

        cmd = f"eeprom_init 0x{addr:02X} {size} {page}"

        self._dut_cmd(cmd, expect_response=False)

    @keyword("I2C Device Should Exist")
    def i2c_device_should_exist(self, resp, addr):

        addr_val = int(str(addr), 0)

        hex_addr = f"0x{addr_val:02X}".lower()

        if hex_addr not in resp.lower():
            raise AssertionError(
                f"I2C device {hex_addr} not detected"
            )

    @keyword("DUT EEPROM Fill")
    def dut_eeprom_fill(self, addr, length):

        addr = self._validate_i2c_addr(addr)
        length = self._validate_uint("length", length)

        self._dut_cmd(
            f"eeprom_fill 0x{addr:02X} {length}",
            expect_response=False
        )

    @keyword("DUT EEPROM Write")
    def dut_eeprom_write(self, addr, length):

        addr = self._validate_i2c_addr(addr)
        length = self._validate_uint("length", length)

        self._dut_cmd(
            f"eeprom_write 0x{addr:02X} {length}",
            expect_response=False
        )

    @keyword("DUT EEPROM Read")
    def dut_eeprom_read(self, addr, length):

        addr = self._validate_i2c_addr(addr)
        length = self._validate_uint("length", length)

        resp = self._dut_cmd(
            f"eeprom_read 0x{addr:02X} {length}"
        )

        # ---- extract ONLY data section ----
        match = re.search(
            r"\)\:\s*\n(.*)",
            resp,
            re.S
        )

        if not match:
            raise AssertionError("EEPROM data block not found")

        data_block = match.group(1)

        # now parse bytes
        data = re.findall(r"\b[0-9A-Fa-f]{2}\b", data_block)

        if len(data) != length:
            raise AssertionError(
                f"Expected {length} bytes, got {len(data)}"
            )

        return data

    @keyword("DUT Write And Verify EEPROM")
    def dut_write_and_verify(self, addr, length):

        logger.console("\nWriting EEPROM...")
        self.dut_eeprom_fill(addr, length)

        logger.console("\nReading EEPROM...")
        data = self.dut_eeprom_read(addr, length)

        logger.info("\nEEPROM data length OK")

        return data
    
    @keyword("Data Should Increment")
    def eeprom_data_should_increment(self, data):

        for i, byte in enumerate(data):
            if int(byte, 16) != (i & 0xFF):
                raise AssertionError(
                    f"Mismatch at {i}: {byte}"
                )
            
    @keyword("DUT Measure EEPROM Write Time")
    def measure_write_time(self, addr, length):

        addr = self._validate_i2c_addr(addr)
        length = self._validate_uint("length", length)

        start = time.perf_counter()

        self._dut_cmd(f"eeprom_write 0x{addr:02X} {length}", False)

        end = time.perf_counter()

        duration = end - start

        logger.info(f"\nEEPROM write time = {duration:.4f}s")

        return duration
    
    @keyword("DUT Write Time Should Be Less Than")
    def write_time_should_be_less_than(self, measured, limit):

        if float(measured) > float(limit):
            raise AssertionError(
                f"Write too slow: {measured}s"
            )

    # ------------------
    # SPI FLASH (W25Q)
    # ------------------
    @keyword("DUT Read SPI Flash ID")
    def read_spi_flash_id(self):
        resp = self._dut_cmd("w25q_ID")
        if "EF4018" in resp:
            return True
        else:
            raise AssertionError(f"Wrong Flash ID! Expected EF4018, got: {resp}")

    @keyword("DUT Read SPI Flash Data")
    def read_spi_flash_data(self, length):
        length = self._validate_uint("length", length)
        resp = self._dut_cmd(f"w25q_read {length}")

        match = re.search(
            r"W25Q\:\s*\n(.*)",
            resp,
            re.S
        )

        if not match:
            raise AssertionError("SPI Flash data block not found in response")

        data_block = match.group(1)

        data = re.findall(r"\b[0-9A-Fa-f]{2}\b", data_block)

        if len(data) != length:
            raise AssertionError(
                f"SPI Flash Read Error: Expected {length} bytes, got {len(data)}"
            )
        return data

    @keyword("DUT Write SPI Flash Data")
    def write_spi_flash_data(self, length):
        length = self._validate_uint("length", length)
        self._dut_cmd(f"w25q_write {length}")

    @keyword("DUT Erase SPI Flash Data")
    def erase_spi_flash_data(self):
        rsp = self._dut_cmd(f"w25q_erasechip")
        if "OK" not in rsp:
            raise AssertionError(f"Failed to erase SPI Flash")

    @keyword("DUT Verify Is SPI Flash Data Erased")
    def is_spi_flash_data_erased(self, data):
        for index, byte in enumerate(data):
            # Ép kiểu về chữ HOA để so sánh cho an toàn (lỡ firmware trả về 'ff')
            if byte.upper() != "FF":
                raise AssertionError(
                    f"Flash is not blank! Found byte '{byte}' at index {index}."
                )
        return True

    # ------------------
    # Generate waveform
    # ------------------
    @keyword("DUT Generate Sine Wave")
    def dut_generate_sine_wave(self, frequency):
        frequency = self._validate_uint("frequency", frequency)
        resp = self._dut_cmd(f"set_freq {frequency}")
        if "Invalid" in resp:
            raise AssertionError(f"Invalid frequency: {frequency}")
        resp = self._dut_cmd("sine_wave 1")
        if "Start" not in resp:
            raise AssertionError(f"DUT failed to generate sine wave")
        return True

    @keyword("DUT Stop Sine Wave")
    def dut_stop_sine_wave(self):
        resp = self._dut_cmd("sine_wave 0")
        if "Stop" not in resp:
            raise AssertionError(f"DUT failed to stop sine wave")
        return True

    @keyword("DUT Generate Triangle Wave")
    def dut_generate_triangle_wave(self, frequency):
        frequency = self._validate_uint("frequency", frequency)
        resp = self._dut_cmd(f"set_freq {frequency}")
        if "Invalid" in resp:
            raise AssertionError(f"Invalid frequency: {frequency}")

        resp = self._dut_cmd("triangle_wave 1")
        if "Start" not in resp:
            raise AssertionError(f"DUT failed to generate triangle wave")
        return True

    @keyword("DUT Stop Triangle Wave")
    def dut_stop_triangle_wave(self):
        resp = self._dut_cmd("triangle_wave 0")
        if "Stop" not in resp:
            raise AssertionError(f"DUT failed to stop triangle wave")
        return True

    # ------------------
    # Analog
    # ------------------
    @keyword("DUT Read ADC Voltage")
    def dut_read_adc_voltage(self, expected_volt=None):

        resp = self._dut_cmd("adc_read")

        match = re.search(r"Voltage:\s*(\d+)\s*mV", resp)
        if not match:
            raise AssertionError("ADC value not found")

        voltage_mv = int(match.group(1))
        if expected_volt is not None:
            # Ép kiểu chuỗi truyền vào thành số thực, sau đó nhân 1000 để đổi ra mV
            expected_mv = float(expected_volt) * 1000.0
            # Tính khoảng sai số 5%
            margin = expected_mv * 0.05
            lower_bound = expected_mv - margin
            upper_bound = expected_mv + margin

            # So sánh trực tiếp giá trị mV thu được với khoảng cho phép
            if voltage_mv < lower_bound or voltage_mv > upper_bound:
                raise AssertionError(
                    f"FAIL: Voltage out of bounds! "
                    f"Got {voltage_mv} mV, Expected {expected_mv} mV \u00B15% "
                    f"(Range: {lower_bound:.0f} mV to {upper_bound:.0f} mV)"
                )
            logger.info(f"Got {voltage_mv} mV, (Range: [{lower_bound:.0f} -> {upper_bound:.0f}] mV)")
        return True
    
    @keyword("DUT Read Sine Frequency")
    def dut_read_sine_frequency(self, expected_freq_hz=None):
        resp = self._dut_cmd("read_sine_freq")
        match = re.search(r"Measured Frequency:\s*(\d+(?:\.\d+)?)\s*Hz", resp)
        if not match:
            raise AssertionError("Frequency value not found in response")

        # Ép kiểu giá trị đọc được sang số thực (float)
        measured_freq = float(match.group(1))

        # Logic so sánh nếu có truyền tần số kỳ vọng vào file .robot
        if expected_freq_hz is not None:
            expected_freq = float(expected_freq_hz)
            
            # Cho phép sai số 5%
            margin = expected_freq * 0.05
            lower_bound = expected_freq - margin
            upper_bound = expected_freq + margin

            # Check trường hợp FAIL trước (Nằm ngoài vùng cho phép)
            if measured_freq < lower_bound or measured_freq > upper_bound:
                raise AssertionError(
                    f"Sine frequency out of bounds! "
                    f"Got {measured_freq} Hz, Expected {expected_freq} Hz \u00B15% "
                    f"(Range: {lower_bound:.1f} Hz to {upper_bound:.1f} Hz)"
                )
            
            # Đã lọt qua được lệnh raise ở trên -> Chắc chắn PASS
            logger.info(
                f"Measured {measured_freq} Hz. "
                f"Expected: {expected_freq} Hz (Range: [{lower_bound:.1f} -> {upper_bound:.1f}] Hz)"
            )

        # Trả về tần số đo được
        return measured_freq
    
    # ------------------
    # RTC (DS1307)
    # ------------------

    def _validate_rtc_time(self, hour, minute, second):
        hour = self._validate_uint("hour", hour)
        minute = self._validate_uint("minute", minute)
        second = self._validate_uint("second", second)

        if hour > 23:
            raise AssertionError("Hour must be 0-23")
        if minute > 59:
            raise AssertionError("Minute must be 0-59")
        if second > 59:
            raise AssertionError("Second must be 0-59")

        return hour, minute, second


    def _is_leap_year(self, year):
        return (year % 4 == 0 and year % 100 != 0) or (year % 400 == 0)


    def _days_in_month(self, month, year):
        days = [31, 28, 31, 30, 31, 30,
                31, 31, 30, 31, 30, 31]

        if month == 2 and self._is_leap_year(year):
            return 29

        return days[month - 1]


    def _validate_rtc_date(self, dow, date, month, year):
        dow = self._validate_uint("day_of_week", dow)
        date = self._validate_uint("date", date)
        month = self._validate_uint("month", month)
        year = self._validate_uint("year", year)

        if dow < 1 or dow > 7:
            raise AssertionError("Day-of-week must be 1-7")

        if month < 1 or month > 12:
            raise AssertionError("Month must be 1-12")

        if year > 99:
            raise AssertionError("Year must be 0-99")

        full_year = 2000 + year

        max_day = self._days_in_month(month, full_year)

        if date < 1 or date > max_day:
            raise AssertionError(
                f"Invalid date {date}/{month}/{full_year}"
            )

        return dow, date, month, year

    @keyword("DUT Init RTC")
    def dut_init_rtc(self):
        resp = self._dut_cmd("rtc_init")

        if "OK" not in resp.upper():
            raise AssertionError(
                f"RTC init failed: {resp}"
            )

        return True


    @keyword("DUT Set RTC Time")
    def dut_set_rtc_time(self, hour, minute, second):

        hour, minute, second = self._validate_rtc_time(
            hour, minute, second
        )

        resp = self._dut_cmd(
            f"rtc_set_time {hour} {minute} {second}"
        )

        if "OK" not in resp.upper():
            raise AssertionError(
                f"RTC set time failed: {resp}"
            )

        return True


    @keyword("DUT Get RTC Time")
    def dut_get_rtc_time(self):

        resp = self._dut_cmd("rtc_get_time")

        match = re.search(
            r"(\d{1,2}):(\d{1,2}):(\d{1,2})",
            resp
        )

        if not match:
            raise AssertionError(
                f"Failed to parse RTC time: {resp}"
            )

        hour = int(match.group(1))
        minute = int(match.group(2))
        second = int(match.group(3))

        return {
            "hour": hour,
            "minute": minute,
            "second": second
        }

    @keyword("RTC Time Should Be Within")
    def rtc_time_should_be_within(
        self,
        rtc_data,
        hour,
        minute,
        second,
        tolerance=2
    ):
        hour = int(hour)
        minute = int(minute)
        second = int(second)
        tolerance = int(tolerance)

        expected = hour * 3600 + minute * 60 + second

        actual = (
            int(rtc_data["hour"]) * 3600 +
            int(rtc_data["minute"]) * 60 +
            int(rtc_data["second"])
        )

        if abs(actual - expected) > tolerance:
            raise AssertionError(
                f"RTC time out of tolerance. "
                f"Expected {hour:02}:{minute:02}:{second:02}, "
                f"Got {rtc_data['hour']:02}:{rtc_data['minute']:02}:{rtc_data['second']:02}"
            )

        return True

    @keyword("DUT Set RTC Date")
    def dut_set_rtc_date(self, dow, date, month, year):

        dow, date, month, year = self._validate_rtc_date(
            dow, date, month, year
        )

        resp = self._dut_cmd(
            f"rtc_set_date {dow} {date} {month} {year}"
        )

        if "OK" not in resp.upper():
            raise AssertionError(
                f"RTC set date failed: {resp}"
            )

        return True


    @keyword("DUT Get RTC Date")
    def dut_get_rtc_date(self):

        resp = self._dut_cmd("rtc_get_date")

        match = re.search(
            r"DOW=(\d+)\s+(\d{1,2})/(\d{1,2})/(\d{4})",
            resp
        )

        if not match:
            raise AssertionError(
                f"Failed to parse RTC date: {resp}"
            )

        return {
            "dow": int(match.group(1)),
            "date": int(match.group(2)),
            "month": int(match.group(3)),
            "year": int(match.group(4))
        }


    @keyword("RTC Date Should Be")
    def rtc_date_should_be(
        self,
        rtc_data,
        dow,
        date,
        month,
        year
    ):

        dow, date, month, year = self._validate_rtc_date(
            dow, date, month, year
        )

        full_year = 2000 + year

        if rtc_data["dow"] != dow:
            raise AssertionError(
                f"DOW mismatch: {rtc_data['dow']} != {dow}"
            )

        if rtc_data["date"] != date:
            raise AssertionError(
                f"Date mismatch: {rtc_data['date']} != {date}"
            )

        if rtc_data["month"] != month:
            raise AssertionError(
                f"Month mismatch: {rtc_data['month']} != {month}"
            )

        if rtc_data["year"] != full_year:
            raise AssertionError(
                f"Year mismatch: {rtc_data['year']} != {full_year}"
            )

        return True

    # ------------------
    # GPIO LED
    # ------------------
    @keyword("DUT Set LED")
    def dut_set_led(self, index, state):
        rsp = self._dut_cmd(f"set_led {index} {state}")
        if "OK" not in rsp:
            raise AssertionError(f"Failed to set LED {index} to {state}")
        return True

    # ------------------
    # UART
    # ------------------
    @keyword("DUT Init UART")
    def dut_init_uart(self):
        resp = self._dut_cmd(f"uart_init")
        if "OK" not in resp:
            raise AssertionError(f"{resp}")
        return True

    @keyword("DUT Send UART String")
    def dut_send_uart_string(self, text):
        resp = self._dut_cmd(f"uart_tx {text}")
        if "OK" not in resp:
            raise AssertionError(f"DUT failed to send UART string")
        return True

    @keyword("DUT Read UART Data")
    def dut_read_uart_data(self):
        resp = self._dut_cmd("uart_rx")

        if "OK" not in resp:
            raise AssertionError("DUT failed to read UART data")

        data = re.findall(r"\b[0-9A-Fa-f]{2}\b", resp)

        if not data:
            raise AssertionError("No UART data found")

        return data

    @keyword("DUT Verify UART String")
    def verify_string(self, received_data, expected_string):

        actual_string = "".join([chr(int(x, 16)) for x in received_data]).strip()
        expected_string = expected_string.strip()

        if actual_string != expected_string:
            raise AssertionError(
                f"String mismatch\n"
                f"Expected: '{expected_string}'\n"
                f"Actual  : '{actual_string}'"
            )