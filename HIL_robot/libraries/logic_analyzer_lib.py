from robot.api.deco import library, keyword
from robot.api import logger
from cli_lib import CLI
import re
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
