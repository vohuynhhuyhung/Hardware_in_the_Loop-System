import serial
import time
import datetime
import numpy as np

class CLI:
    """
    Generic CLI communication class.

    Responsibility:
        - open serial
        - send command
        - wait response
    """

    def __init__(self):
        self.ser = None

    # -------------------------
    # CONNECTION
    # -------------------------

    def connect(self, port, baud):

        self.ser = serial.Serial(port, baud, timeout=0.1)

        # wait STM32 reboot
        time.sleep(0.5)

        # clear boot garbage
        self.ser.reset_input_buffer()

        # wake CLI
        self.ser.write(b"\n")

    def disconnect(self):
        """Close serial"""
        if self.ser:
            self.ser.close()

    # -------------------------
    # COMMAND
    # -------------------------

    def execute(self, cmd):

        if not self.ser:
            raise Exception("Serial not connected")

        self.ser.reset_input_buffer()

        self.ser.write((cmd + "\r\n").encode())

        response = ""
        timeout = time.time() + 3

        while time.time() < timeout:

            if self.ser.in_waiting:
                data = self.ser.read(self.ser.in_waiting).decode(errors="ignore")
                response += data
                # Chuyển chuỗi thành chữ hoa để check không phân biệt hoa/thường
                if "OK" in response.upper():
                    time.sleep(0.2)
                    if self.ser.in_waiting:
                        response += self.ser.read(self.ser.in_waiting).decode(errors="ignore")
                    break
            time.sleep(0.2)

        return response.strip()
    
    # -------------------------
    # LOGIC ANALYZER / BINARY
    # -------------------------

    def setup_logic_analyzer(self, rate: int, samples: int):
        if not self.ser:
            raise Exception("Serial not connected")

        # 1. Reset state
        self.ser.write(b"*")
        time.sleep(0.05)

        # 2. Cấu hình các kênh Analog / Digital
        channels = [b"A10\n", b"D10\n", b"D11\n", b"D12\n", b"D13\n", b"D14\n", b"D15\n", b"D16\n"]
        for ch in channels:
            self.ser.write(ch)
            time.sleep(0.01)

        # 3. Cấu hình Rate và Sample count
        self.ser.write(f"R{rate}\n".encode('ascii'))
        time.sleep(0.01)
        self.ser.write(f"L{samples}\n".encode('ascii'))
        time.sleep(0.01)

    # Dump data receiced from logic anlyzer for DEBUG purpose
    def dump_to_bin_file(self, raw_data: bytes, prefix="logic_debug"):
        """
        Lưu toàn bộ mảng byte thô ra file .bin để phân tích (dùng HxD hoặc script Python).
        Tên file tự động gắn timestamp để không bị ghi đè.
        """
        if not raw_data:
            print("[DEBUG] Không có data để lưu.")
            return

        # Tạo tên file có ngày giờ (Ví dụ: logic_debug_20260328_153000.bin)
        timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"{prefix}_{timestamp}.bin"

        try:
            with open(filename, "wb") as f:
                f.write(raw_data)
            print(f"[DEBUG] Đã lưu {len(raw_data)} bytes vào file: {filename}")
        except Exception as e:
            print(f"[DEBUG] Lỗi khi lưu file bin: {e}")

    # -------------------------
    # Collect data stream from logic analyzer until '+' flag or timeout, return raw bytes
    # -------------------------
    def capture_binary_stream(self, timeout_sec: float) -> bytes:
        """
        Gửi lệnh 'F' để kích hoạt Analyzer và thu thập toàn bộ mảng Byte 
        cho đến khi gặp cờ '+' hoặc hết thời gian timeout.
        """
        if not self.ser:
            raise Exception("Serial not connected")

        self.ser.reset_input_buffer()
        
        # Gửi lệnh bắt đầu
        self.ser.write(b"F\n")

        buffer = bytearray()
        end_time = time.time() + timeout_sec

        while time.time() < end_time:
            try:
                if self.ser.in_waiting:
                    chunk = self.ser.read(self.ser.in_waiting)
                    buffer.extend(chunk)
                    
                    # Kiểm tra cờ '+'
                    if b'+' in chunk:
                        break
            except serial.SerialException as e:
                print(f"[LỖI CỔNG COM] Ngắt kết nối đột ngột khi đang hứng data: {e}")
                break
            
            # Ngủ cực ngắn để không làm kẹt CPU nhưng vẫn bắt sóng kịp thời
            time.sleep(0.001) 

        raw_array = np.frombuffer(buffer, dtype=np.uint8)
        clean_array = raw_array[raw_array >= 0x80] # Lọc data của logic vì data analyzer luôn có MSB=1 (>=0x80)
        final_data = clean_array.tobytes()
        
        # Gọi hàm ghi file debug tại đây trước khi trả về (DEBUG purpose)
        self.dump_to_bin_file(bytes(buffer), prefix="logic_raw")  # data thô, chưa lọc
        self.dump_to_bin_file(final_data, prefix="logic_filtered")  # data sau khi lọc >= 0x80

        return final_data
