# File: ui/main_window.py
import time
import serial 
import serial.tools.list_ports
from PyQt5.QtWidgets import (QMainWindow, QWidget, QVBoxLayout, QGroupBox, 
                             QPushButton, QTextEdit, QSplitter, QLabel, 
                             QComboBox, QCheckBox, QTabWidget, QHBoxLayout, QLineEdit)
from PyQt5.QtCore import Qt, QTimer
from PyQt5.QtGui import QIcon

from core.uart_thread import UARTThread
# from ui.tabs.memory_tab import MemoryTab
from ui.tabs.logic_tab import LogicTab
from ui.tabs.peripheral_tab import PeripheralTab

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("HIL Simulation UI")
        # Ensure path is correct relative to where main.py runs
        self.setWindowIcon(QIcon("./App/App_PyQt5/img/logoBK.png"))

        self.uart_dut = UARTThread("DUT")
        self.uart_hil = UARTThread("HIL")
        self.uart_logic = UARTThread("LOGIC")

        # Init UI elements
        splitter = QSplitter(Qt.Horizontal)
        self.setCentralWidget(splitter)

        # Setup Tabs
        # self.memory_tab = MemoryTab(self.uart_dut, self.uart_hil)
        self.logic_tab = LogicTab(self.uart_logic)
        self.peripheral_tab = PeripheralTab(self.uart_dut, self.uart_hil)

        # Build Main View
        splitter.addWidget(self.build_left())
        splitter.addWidget(self.build_center())
        splitter.setSizes([500, 1450])

        # Connect Signals -> Tabs & MainWindow
        self.uart_dut.log_signal.connect(self.append_dut_log)
        self.uart_hil.log_signal.connect(self.append_hil_log)
        self.uart_logic.data_signal.connect(self.logic_tab.process_raw_data)

    def build_left(self):
        left_tabs = QTabWidget()

        # TAB 1: COM
        tab_com = QWidget()
        lay_com = QVBoxLayout(tab_com)
        lay_com.addWidget(self.create_uart_control("HIL", self.uart_hil, needs_scan=True, expected_id="HIL"))
        lay_com.addWidget(self.create_uart_control("LOGIC", self.uart_logic, needs_scan=True, expected_id="SRPICO"))
        lay_com.addWidget(self.create_uart_control("DUT", self.uart_dut, needs_scan=False))
        
        # ===============================================
        # KHỐI ĐIỀU KHIỂN NGUỒN DUT (POWER CONTROL)
        # ===============================================
        power_box = QGroupBox("DUT Power Control")
        power_lay = QHBoxLayout(power_box)
        
        # Đèn trạng thái nguồn (Tròn)
        self.power_indicator = QLabel()
        self.power_indicator.setFixedSize(20, 20)
        self.set_power_indicator_unknown() # Mặc định là màu xám (chưa biết trạng thái)
        
        # Các nút bấm
        self.btn_power_on = QPushButton("ON")
        self.btn_power_off = QPushButton("OFF")
        self.btn_power_status = QPushButton("Status")

        # Chỉ enable nút bấm khi HIL connect
        self.btn_power_on.setEnabled(False)
        self.btn_power_off.setEnabled(False)
        
        # Setup UI layout cho khối nguồn
        power_lay.addWidget(self.btn_power_on)
        power_lay.addWidget(self.btn_power_off)
        power_lay.addStretch()
        power_lay.addWidget(self.power_indicator)
        
        # Gán sự kiện tạm thời để đổi màu đèn
        self.btn_power_on.clicked.connect(
            lambda: self.uart_hil.send("dut_power 1")
        )

        self.btn_power_off.clicked.connect(
            lambda: self.uart_hil.send("dut_power 0")
        )

        lay_com.addWidget(power_box)
        # ===============================================

        lay_com.addStretch()  

        # ===============================================
        # TAB 2: TERMINAL (Đã đổi tên thứ tự từ 3 thành 2)
        # ===============================================
        tab_term = QWidget()
        lay_term = QVBoxLayout(tab_term)
        term_splitter = QSplitter(Qt.Vertical)

        self.dut_term_box, self.dut_log_text = self.create_terminal_box("DUT Terminal", self.uart_dut)
        self.hil_term_box, self.hil_log_text = self.create_terminal_box("HIL Terminal", self.uart_hil)

        term_splitter.addWidget(self.dut_term_box)
        term_splitter.addWidget(self.hil_term_box)
        term_splitter.setSizes([500, 500])

        lay_term.addWidget(term_splitter)

        left_tabs.addTab(tab_com, "Serial")
        left_tabs.addTab(tab_term, "Terminal")
        return left_tabs

    def build_center(self):
        tabs = QTabWidget()
        tabs.addTab(self.peripheral_tab, "Generator")
        tabs.addTab(self.logic_tab, "Collector")
        # tabs.addTab(self.memory_tab, "Memory Emu")
        return tabs

    # ================== UI HELPER WIDGETS ==================
    def create_terminal_box(self, title, uart):
        # Giữ nguyên y hệt design gốc của bạn (Nền trắng, text đen)
        box = QGroupBox(title)
        lay = QVBoxLayout(box)
        text = QTextEdit()
        text.setReadOnly(True)
        text.setStyleSheet("background:white;font-family:Consolas;font-size:9pt;")

        bottom = QHBoxLayout()
        input_line = QLineEdit()
        input_line.setPlaceholderText("Enter command and press Enter")
        btn_clear = QPushButton("Clear")
        btn_clear.setFixedWidth(70)

        btn_clear.clicked.connect(text.clear)
        input_line.returnPressed.connect(lambda: (uart.send(input_line.text()), input_line.clear()))

        bottom.addWidget(input_line)
        bottom.addWidget(btn_clear)

        lay.addWidget(text)
        lay.addLayout(bottom)
        return box, text

    def create_uart_control(self, name, uart, needs_scan=False, expected_id=""):
        box = QGroupBox(f"{name} Serial Port")
        lay = QVBoxLayout(box)
        cb_port = QComboBox()
        cb_baud = QComboBox()
        cb_baud.addItems(["9600", "19200", "38400", "57600", "115200","921600"])
        cb_baud.setCurrentText("115200")

        btn_refresh = QPushButton("Refresh COM")
        btn_scan = QPushButton("Scan Device") if needs_scan else None
        btn_connect = QPushButton("Connect")

        # Khối logic Scan Device
        if needs_scan:
            btn_connect.setEnabled(False) # Khóa nút Connect ban đầu

            # Nếu đổi port khác -> Khóa lại bắt Scan từ đầu
            def on_port_changed():
                if not uart.isRunning():
                    btn_connect.setEnabled(False)
            cb_port.currentTextChanged.connect(on_port_changed)

            def perform_scan():
                port = cb_port.currentText()
                if not port: return
                baud = int(cb_baud.currentText())
                
                # Không được phép scan khi luồng Thread đang chiếm dụng cổng
                if uart.isRunning():
                    return

                try:
                    # Mở cổng tạm thời để bắn data
                    with serial.Serial(port, baud, timeout=0.5) as s:
                        s.reset_input_buffer()
                        s.write(b"i\r\n")
                        s.flush()
                        time.sleep(0.1) # Chờ mạch trả lời
                        resp = s.read(100).decode(errors='ignore').strip()

                        if expected_id:
                            if expected_id in resp:
                                btn_connect.setEnabled(True)
                            else:
                                btn_connect.setEnabled(False)
                        else:
                            # HIL chưa cần Verify Data
                            btn_connect.setEnabled(True)

                except Exception as e:
                    btn_connect.setEnabled(False)

            btn_scan.clicked.connect(perform_scan)

        def refresh_ports():
            current = cb_port.currentText()
            cb_port.clear()
            cb_port.addItems([p.device for p in serial.tools.list_ports.comports()])
            if current in [cb_port.itemText(i) for i in range(cb_port.count())]:
                cb_port.setCurrentText(current)

        refresh_ports()

        def toggle():
            if uart.isRunning():
                uart.close()
                uart.wait()
            else:
                uart.open(cb_port.currentText(), int(cb_baud.currentText()))

        def update_connect_button():
            connected = uart.isRunning()

            btn_connect.setText(
                "Disconnect" if connected else "Connect"
            )

            cb_port.setEnabled(not connected)
            cb_baud.setEnabled(not connected)

            if needs_scan and btn_scan:
                btn_scan.setEnabled(not connected)

            hil_connected = self.uart_hil.isRunning()

            self.btn_power_on.setEnabled(hil_connected)
            self.btn_power_off.setEnabled(hil_connected)

            if not hil_connected:
                self.set_power_indicator_unknown()

        # Cập nhật trạng thái nút Connect mỗi 1s để phản ánh đúng trạng thái kết nối (đặc biệt khi rút cáp đột ngột)
        timer = QTimer(box)
        timer.timeout.connect(update_connect_button)
        timer.start(1000)

        # Khi rút cáp, luồng bị lỗi và dừng lại, nó sẽ tự động kích hoạt tín hiệu 'finished'
        uart.finished.connect(lambda: btn_connect.setText("Connect"))

        btn_refresh.clicked.connect(refresh_ports)
        btn_connect.clicked.connect(toggle)

        lay.addWidget(QLabel("COM Port"))
        
        # 2 nút Refresh và Scan vào chung một hàng
        port_lay = QHBoxLayout()
        port_lay.setContentsMargins(0, 0, 0, 0)
        port_lay.addWidget(cb_port)
        port_lay.addWidget(btn_refresh)
        if needs_scan:
            port_lay.addWidget(btn_scan)
        lay.addLayout(port_lay)

        lay.addWidget(QLabel("Baudrate"))
        lay.addWidget(cb_baud)
        lay.addWidget(btn_connect)
        return box

    def set_power_indicator_unknown(self):
        self.power_indicator.setStyleSheet(
            "background-color: #95a5a6; "
            "border-radius: 10px; "
            "border: 1px solid #7f8c8d;"
        )

    def set_power_indicator_state(self, is_on):
        """Hàm cập nhật màu sắc cho đèn báo nguồn (Xanh = ON, Đỏ = OFF)"""
        if is_on:
            self.power_indicator.setStyleSheet(
                "background-color: #2ecc71; border-radius: 10px; border: 1px solid #27ae60;"
            )
        else:
            self.power_indicator.setStyleSheet(
                "background-color: #e74c3c; border-radius: 10px; border: 1px solid #c0392b;"
            )

    # ================= UI LOGGING SLOTS =================
    def append_dut_log(self, text):
        self.dut_log_text.append(text)
        self.dut_log_text.moveCursor(self.dut_log_text.textCursor().End)

    def append_hil_log(self, text):
        self.hil_log_text.append(text)
        self.hil_log_text.moveCursor(
            self.hil_log_text.textCursor().End
        )

        # ===== DUT POWER STATUS =====
        if "ON" in text:
            self.set_power_indicator_state(True)

        elif "OFF" in text:
            self.set_power_indicator_state(False)

    def closeEvent(self, e):
        self.uart_dut.close()
        self.uart_hil.close()
        self.uart_logic.close()
        self.uart_dut.wait()
        self.uart_hil.wait()
        self.uart_logic.wait()
        e.accept()