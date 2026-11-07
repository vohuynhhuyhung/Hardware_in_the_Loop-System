# File: core/uart_thread.py

import time
import serial
from PyQt5.QtCore import QThread, pyqtSignal
from core.parsers import SPIParser, I2CParser, UARTParser

class UARTThread(QThread):
    log_signal = pyqtSignal(str)
    data_signal = pyqtSignal(bytes)
    spi_data = pyqtSignal(int, int)
    i2c_data = pyqtSignal(int, int)
    uart_data = pyqtSignal(int, int)
    spi_clear = pyqtSignal()
    i2c_clear = pyqtSignal()
    uart_clear = pyqtSignal()
    pc_log_signal = pyqtSignal(str)
    test_completed = pyqtSignal(str)

    def __init__(self, name):
        super().__init__()
        self.name = name
        self.running = False
        self.ser = None
        self.spi_parser = SPIParser(self.spi_data.emit, self.log_signal.emit, self.spi_clear.emit, self.pc_log_signal.emit, self.test_completed.emit)
        self.i2c_parser = I2CParser(self.i2c_data.emit, self.log_signal.emit, self.i2c_clear.emit, self.test_completed.emit)
        # self.uart_parser = UARTParser(
        #     byte_cb=self.handle_uart_byte,
        #     log_cb=self.log_signal.emit
        # )
    def open(self, port, baud):
        self.port = port
        self.baud = baud
        self.running = True
        self.start()

    def run(self):
        try:
            # Vẫn ép buffer OS lên mức cao nhất để hứng đạn thay Python
            self.ser = serial.Serial(self.port, self.baud, timeout=0.01)
            if hasattr(self.ser, 'set_buffer_size'):
                try: self.ser.set_buffer_size(rx_size=1048576) 
                except Exception: pass

            self.log_signal.emit(f"[{self.name}] Connected {self.port} @ {self.baud}")

            # Xô đựng data chuẩn bị gửi ra UI (Frame buffer)
            frame_buffer = bytearray()
            text_buffer = ""
            
            last_emit_time = time.time()
            # Giới hạn tốc độ vẽ: 0.05 giây = 20 khung hình/giây (20 FPS)
            # Bạn có thể giảm xuống 0.03 (30 FPS) nếu máy tính PC của bạn đủ mạnh
            EMIT_INTERVAL = 0.05 

            while self.running:
                if self.name == "LOGIC":
                    # 1. HÚT SIÊU TỐC KHÔNG CHỜ ĐỢI
                    raw_chunk = self.ser.read(32768) 
                    
                    if raw_chunk:
                        # Bỏ data vừa hút vào xô
                        frame_buffer.extend(raw_chunk)
                        current_time = time.time()
                        
                        # 2. KIỂM TRA ĐÃ ĐẾN LÚC BÁO UI VẼ CHƯA?
                        if (current_time - last_emit_time) >= EMIT_INTERVAL:
                            # Quăng toàn bộ data trong xô ra cho UI vẽ
                            self.data_signal.emit(bytes(frame_buffer))
                            
                            # Quăng xong thì dọn sạch xô để vòng lặp sau hứng tiếp
                            frame_buffer.clear()
                            last_emit_time = current_time

                        # 3. KHI PICO CHỐT SỔ (Dấu '+')
                        if b'+' in raw_chunk:
                            # Quăng nốt những giọt data cuối cùng còn kẹt trong xô (nếu có)
                            if frame_buffer:
                                self.data_signal.emit(bytes(frame_buffer))
                                frame_buffer.clear()
                            self.log_signal.emit("[LOGIC] Đã tải xong toàn bộ dữ liệu.")
                else:
                    # Kênh Text giữ nguyên logic mượt mà
                    if self.ser.in_waiting:
                        raw_chunk = self.ser.read(self.ser.in_waiting)
                        text_buffer += raw_chunk.decode(errors="ignore")
                        while '\n' in text_buffer:
                            line, text_buffer = text_buffer.split('\n', 1)
                            line = line.strip()
                            if line: self.handle_line(line)
                    else:
                        time.sleep(0.001)
        except Exception:
            pass
        finally:
            if self.ser:
                self.ser.close()
            self.log_signal.emit(f"[{self.name}] Disconnected")

    def close(self):
        self.running = False

    def send(self, text):
        if self.ser and self.ser.is_open:
            self.ser.write((text + "\r\n").encode())

    def handle_line(self, line):
        self.log_signal.emit(f"{line}")
        line = line.strip()

        if self.spi_parser.feed(line): return
        if self.i2c_parser.feed(line): return
    #     if self.uart_parser.feed(line): return

    # def handle_uart_byte(self, byte):
    #     self.pc_log_signal.emit(f"UART_BYTE:{byte}")
