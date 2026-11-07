# File: ui/tabs/memory_tab.py
import time
from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QGroupBox, 
                             QSplitter, QTableWidget, QTableWidgetItem, QHeaderView, 
                             QPushButton, QTextEdit, QLineEdit, QCheckBox)
from PyQt5.QtCore import Qt, QTimer
from PyQt5.QtGui import QBrush, QColor

class MemoryTab(QWidget):
    def __init__(self, uart_dut, uart_hil):
        super().__init__()
        self.uart_dut = uart_dut
        self.uart_hil = uart_hil

        # --- Test Variables ---
        self.test_queue = []
        self.current_test = None
        self.is_running_tests = False  

        # ================= CHIA ĐÔI MÀN HÌNH =================
        main_splitter = QSplitter(Qt.Horizontal)
        
        # --- CỘT TRÁI: MEMORY TABLES ---
        left_widget = QWidget()
        left_layout = QVBoxLayout(left_widget)
        left_layout.setContentsMargins(0, 0, 10, 0)
        
        mem_splitter = QSplitter(Qt.Vertical)
        self.spi_table = self.create_mem_table("SPI Flash")
        self.i2c_table = self.create_mem_table("I2C EEPROM")

        mem_splitter.addWidget(self.spi_table[0])
        mem_splitter.addWidget(self.i2c_table[0])
        mem_splitter.setSizes([500, 500])
        
        left_layout.addWidget(mem_splitter)

        # --- CỘT PHẢI: EMULATION DEMOS ---
        right_widget = QWidget()
        right_layout = QVBoxLayout(right_widget)
        right_layout.setContentsMargins(0, 0, 0, 0)
        
        test_splitter = QSplitter(Qt.Vertical)

        test_box = QGroupBox("HIL Emulation Demos")
        tlay = QVBoxLayout(test_box)
        self.cb_spi = QCheckBox("SPI Flash Demo")
        self.cb_i2c = QCheckBox("I2C EEPROM Demo")
        btn_run = QPushButton("Run Selected Demos")
        btn_run.clicked.connect(self.run_tests)

        tlay.addWidget(self.cb_spi)
        tlay.addWidget(self.cb_i2c)
        tlay.addStretch()
        tlay.addWidget(btn_run)

        bottom_widget = QWidget()
        bottom_lay = QVBoxLayout(bottom_widget)
        bottom_lay.setContentsMargins(0, 0, 0, 0) 
        self.pc_log = self.create_pc_log_box("Demo Log")
        bottom_lay.addWidget(self.pc_log[0])

        test_splitter.addWidget(test_box)
        test_splitter.addWidget(bottom_widget)
        test_splitter.setSizes([200, 800]) 
        
        right_layout.addWidget(test_splitter)

        # Nạp vào main splitter
        main_splitter.addWidget(left_widget)
        main_splitter.addWidget(right_widget)
        main_splitter.setSizes([700, 300]) # Bảng memory to hơn bảng test

        layout = QVBoxLayout(self)
        layout.addWidget(main_splitter)

        # Connect Signals for Memory Updates
        self.uart_dut.spi_data.connect(self.update_spi)
        self.uart_dut.spi_clear.connect(self.clear_spi_table)
        self.uart_dut.i2c_data.connect(self.update_i2c)
        self.uart_dut.i2c_clear.connect(self.clear_i2c_table)
        self.uart_dut.spi_parser.highlight_error_cb = self.highlight_spi_error

        # Connect Signals for Testing
        self.uart_dut.pc_log_signal.connect(self.pc_log_append)
        self.uart_dut.test_completed.connect(self.on_test_completed)

    def create_mem_table(self, title):
        box = QGroupBox(title)
        lay = QVBoxLayout(box)
        table = QTableWidget(64, 16)
        table.setHorizontalHeaderLabels([f"{i:X}" for i in range(16)])
        table.setVerticalHeaderLabels([f"{i*16:04X}" for i in range(64)])
        table.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)
        lay.addWidget(table)
        return box, table

    def create_pc_log_box(self, title):
        box = QGroupBox(title)
        lay = QVBoxLayout(box)
        text = QTextEdit()
        text.setReadOnly(True)
        text.setStyleSheet("background: white; font-family:Consolas; font-size:8pt;")
        btn_clear = QPushButton("Clear")
        btn_clear.clicked.connect(text.clear)
        lay.addWidget(text)
        lay.addWidget(btn_clear)
        return box, text

    def pc_log_append(self, text, newline=True):
        cursor = self.pc_log[1].textCursor()
        cursor.movePosition(cursor.End)
        if newline: cursor.insertText(text + "\n")
        else: cursor.insertText(text)
        self.pc_log[1].setTextCursor(cursor)

    # ================= UI UPDATE SLOTS =================
    def update_spi(self, addr, val):
        if 0 <= addr < 1024:
            r, c = divmod(addr, 16)
            self.spi_table[1].setItem(r, c, QTableWidgetItem(f"{val:02X}"))

    def update_i2c(self, addr, val):
        if 0 <= addr < 1024:
            r, c = divmod(addr, 16)
            self.i2c_table[1].setItem(r, c, QTableWidgetItem(f"{val:02X}"))

    def highlight_spi_error(self, addr):
        if 0 <= addr < 1024:
            r, c = divmod(addr, 16)
            item = self.spi_table[1].item(r, c)
            if item:
                item.setBackground(QBrush(QColor(255, 0, 0, 200))) 

    def clear_spi_table(self):
        self.spi_table[1].clearContents()

    def clear_i2c_table(self):
        self.i2c_table[1].clearContents()

    # ==================== TEST LOGIC ====================
    def run_tests(self):
        self.test_queue = []
        self.is_running_tests = True

        if self.cb_spi.isChecked():
            self.test_queue.extend([
                self.spi_flash_test_id,
                self.spi_flash_test_prepare_mem,
                self.spi_flash_test_erase,
                self.spi_flash_test_write
            ])
        if self.cb_i2c.isChecked():
            self.test_queue.append(self.run_i2c_eeprom_test)

        if not self.test_queue:
            self.pc_log_append("No demo selected")
            self.is_running_tests = False
            return
        self.run_next_test()

    def run_next_test(self):
        if not self.test_queue:
            self.pc_log_append("=== ALL DEMOS DONE ===")
            self.is_running_tests = False 
            return
        self.current_test = self.test_queue.pop(0)
        self.current_test()

    def run_next_test_delayed(self, delay_ms=5000):
        QTimer.singleShot(delay_ms, self.run_next_test)

    def spi_flash_test_id(self):
        self.pc_log_append("=== SPI Flash Demo ===")
        self.pc_log_append("[DUT] Read ID -> ", newline=False)
        self.uart_dut.send("w25q_ID")

    def spi_flash_test_write(self):
        self.pc_log_append("[DUT] Write Test -> ", newline=False)
        self.uart_dut.send("w25q_write 1024")
        self.pc_log_append("[DUT] Read & Verify -> ", newline=False)
        self.uart_dut.send("w25q_read 1024")

    def spi_flash_test_prepare_mem(self):
        self.pc_log_append("[HIL] Prepare Memory -> ", newline=False)
        self.uart_dut.spi_parser.set_prepare_mem_size(256)
        self.uart_hil.send("w25q_prepare_mem 256")
        time.sleep(0.5)
        self.pc_log_append("[DUT] Read & Verify -> ", newline=False)
        self.uart_dut.send("w25q_read 256")

    def spi_flash_test_erase(self):
        self.pc_log_append("[DUT] Erase Chip -> ", newline=False)
        self.uart_dut.spi_parser.set_erase_test_data()
        self.uart_dut.send("w25q_erasechip")
        time.sleep(0.5)
        self.pc_log_append("[DUT] Read & Verify -> ", newline=False)
        self.uart_dut.send("w25q_read 1024")

    def run_i2c_eeprom_test(self):
        self.pc_log_append("=== I2C EEPROM DEMO START ===")
        self.i2c_devices = [0x50]
        self.i2c_dev_index = 0
        self.i2c_step = 0
        self.run_i2c_step()

    def run_i2c_step(self):
        if self.i2c_dev_index >= len(self.i2c_devices):
            self.pc_log_append("=== I2C EEPROM DEMO DONE ===")
            self.run_next_test()
            return

        dev = self.i2c_devices[self.i2c_dev_index]
        if self.i2c_step == 0:
            self.pc_log_append(f"[HIL] Init EEPROM 0x{dev:02X}")
            self.uart_hil.send(f"eeprom_init 0x{dev:02X} 1024 256")
            QTimer.singleShot(2000, self.run_i2c_step)
        elif self.i2c_step == 1:
            self.pc_log_append(f"[HIL] Active EEPROM 0x{dev:02X}")
            self.uart_hil.send(f"i2c_dev_active 0x{dev:02X}")
            QTimer.singleShot(2000, self.run_i2c_step)
        elif self.i2c_step == 2:
            self.pc_log_append(f"[DUT] Init EEPROM 0x{dev:02X}")
            self.uart_dut.send(f"eeprom_init 0x{dev:02X} 1024 256")
            QTimer.singleShot(2000, self.run_i2c_step)
        elif self.i2c_step == 3:
            self.pc_log_append(f"[DUT] Fill EEPROM 0x{dev:02X}")
            self.uart_dut.send(f"eeprom_fill 0 256")
            QTimer.singleShot(2000, self.run_i2c_step)
        elif self.i2c_step == 4:
            self.pc_log_append(f"[DUT] Read EEPROM 0x{dev:02X}")
            self.uart_dut.send(f"eeprom_read 0 256")
        elif self.i2c_step == 5:
            self.pc_log_append(f"[HIL] Deinit EEPROM 0x{dev:02X}")
            self.uart_hil.send(f"eeprom_deinit 0x{dev:02X}")
            self.i2c_dev_index += 1
            self.i2c_step = -1
            QTimer.singleShot(2000, self.run_i2c_step)

        self.i2c_step += 1

    def on_test_completed(self, test_type):
        if not self.is_running_tests: return
        
        if test_type == "i2c":
            parser = self.uart_dut.i2c_parser
            if not parser.buffer: self.pc_log_append("FAIL (no data)")
            elif len(parser.buffer) != parser.expected: self.pc_log_append("FAIL (size mismatch)")
            else: self.pc_log_append("PASS")
            parser.reset()
            self.run_i2c_step()
            return

        self.uart_dut.spi_parser.reset()
        if (hasattr(self, 'current_test') and self.current_test and 
            self.current_test.__name__ in ['spi_flash_test_prepare_mem', 'spi_flash_test_erase']):
            self.run_next_test_delayed(5000)
        else:
            self.run_next_test()