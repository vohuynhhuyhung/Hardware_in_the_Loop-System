# File: ui/tabs/logic_tab.py
from PyQt5.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QGroupBox, QPushButton, QLabel, QComboBox
from PyQt5.QtCore import Qt, QTimer
from PyQt5.QtGui import QIcon, QPixmap, QPainter, QColor
import pyqtgraph as pg
import time
import numpy as np

# ==================================================
# CLASS TẠO LABEL KÊNH BẰNG THUẦN PYTHON (KHÔNG HTML)
# ==================================================
class ChannelLabel(pg.LabelItem):
    def __init__(self, text, bg_color):
        # Khởi tạo LabelItem cơ bản với chữ trắng, in đậm
        super().__init__(text=text, color='w', bold=True)
        self.bg_color = QColor(bg_color)
        
        # Cố định chiều rộng để các khung đều nhau
        self.setFixedWidth(40)
        
    def paint(self, p, *args):
        # 1. Vẽ khung nền (Background) trước
        p.setRenderHint(QPainter.Antialiasing)
        p.setPen(Qt.NoPen)
        p.setBrush(self.bg_color)
        
        # Lấy kích thước thực tế của vùng chứa nhãn và tạo viền bo góc
        rect = self.boundingRect().adjusted(2, 2, -2, -2)
        p.drawRoundedRect(rect, 3, 3)
        
        # 2. Gọi hàm vẽ chữ mặc định đè lên trên nền
        super().paint(p, *args)

class LogicTab(QWidget):
    def __init__(self, uart_logic):
        super().__init__()
        
        self.uart_logic = uart_logic
        self.is_running = False

        # --- BỘ ĐỆM VÀ QUẢN LÝ DỮ LIỆU ---
        self.raw_buffer = bytearray()
        self.leftover_byte = bytearray() # Lưu byte bị kẹt lại chưa có cặp
        self.current_idx = 0 # Con trỏ mảng hiện tại
        
        self.current_sample_rate = 100000
        self.expected_samples = 0
        
        # Mảng dữ liệu thời gian và điện áp (cần cho việc tính toán vị trí chuột)
        self.time_arr = np.array([])
        self.a0_arr = np.array([])

        # --- BIẾN QUẢN LÝ CÔNG CỤ ĐO TẦN SỐ ĐỘC LẬP CHO TỪNG KÊNH ---
        self.measure_regions = {}
        self.measure_texts = {}

        self.plot_timer = QTimer()
        self.plot_timer.timeout.connect(self.update_plots)

        main_layout = QVBoxLayout(self)
        main_layout.addWidget(self.build_control_panel())
        main_layout.addWidget(self.build_plot_area())

    # ==================================================
    # VẼ ĐÈN LED CHO NÚT BẤM
    # ==================================================
    def create_led_icon(self, color_hex):
        pixmap = QPixmap(16, 16)
        pixmap.fill(Qt.transparent) 
        
        painter = QPainter(pixmap)
        painter.setRenderHint(QPainter.Antialiasing) 
        painter.setBrush(QColor(color_hex))
        painter.setPen(Qt.NoPen)
        painter.drawEllipse(2, 2, 12, 12) 
        painter.end()
        
        return QIcon(pixmap)

    def build_control_panel(self):
        box = QGroupBox("Sampling Controls")
        layout = QHBoxLayout(box)
        layout.setContentsMargins(10, 10, 10, 10)

        # --- NÚT START / STOP VỚI ĐÈN LED ---
        self.btn_start = QPushButton(" START")
        self.btn_start.setMinimumWidth(100)
        self.btn_start.setStyleSheet("font-weight: bold; padding: 5px;")
        self.btn_start.setIcon(self.create_led_icon("gray"))
        self.btn_start.clicked.connect(self.toggle_start_stop)
        
        layout.addWidget(self.btn_start)
        layout.addSpacing(20)

        # --- CẤU HÌNH SAMPLES ---
        self.cb_samples = QComboBox()
        self.cb_samples.addItems(["10 K", "100 K", "500 K", "1 M", "2.5 M", "3 M", "10 M", "50 M"])
        self.cb_samples.setCurrentText("3 M")

        layout.addWidget(QLabel("Samples:"))
        layout.addWidget(self.cb_samples)
        layout.addSpacing(20)
        
        # --- CẤU HÌNH SAMPLE RATE ---
        self.cb_rate = QComboBox()
        self.cb_rate.addItems(["10 kHz", "100 kHz", "150 kHz", "200 kHz", "250 kHz", "300 kHz"])
        self.cb_rate.setCurrentText("300 kHz")

        layout.addWidget(QLabel("Sample Rate:"))
        layout.addWidget(self.cb_rate)

        # Đẩy tất cả sang trái
        layout.addStretch() 

        return box

    def build_plot_area(self):
        self.plot_widget = pg.GraphicsLayoutWidget()
        self.plot_widget.setBackground('#FFFFFF') 
        
        # Xóa khoảng cách giữa các đồ thị
        self.plot_widget.ci.layout.setSpacing(0)
        self.plot_widget.ci.layout.setContentsMargins(0, 0, 0, 0)

        channels = ['D0', 'D1', 'D2', 'D3', 'D4', 'D5', 'D6', 'A0']
        
        colors = {
            'D0': '#333333', 'D1': '#8B4513', 'D2': '#DC143C', 
            'D3': '#D2691E', 'D4': '#800080', 'D5': '#008080', 
            'D6': '#2E8B57', 'A0': '#4682B4'
        }

        self.plots = {}
        self.curves = {} # [UPDATE] Chuẩn bị sẵn dictionary chứa đường vẽ đồ thị
        self.v_lines = []
        p0 = None

        for i, ch in enumerate(channels):
            # 1. Label Tên kênh bằng Class ChannelLabel (Thuần Python)
            lbl = ChannelLabel(ch, colors[ch])
            self.plot_widget.addItem(lbl, col=0)

            # 2. Biểu đồ (Plot)
            p = self.plot_widget.addPlot(col=1)
            
            # ==========================================
            # TỐI ƯU HÓA PYQTGRAPH CHO LOGIC ANALYZER
            # ==========================================
            p.setClipToView(True) # Chỉ vẽ những điểm nằm trong khung hình hiện tại
            p.setDownsampling(auto=True, mode='peak') # Ghép điểm ảnh, loại bỏ lag
            
            p.getViewBox().setBorder(pg.mkPen(color='#CCCCCC', width=1))

            p.hideButtons()
            
            p.hideAxis('left')
            p.setMouseEnabled(y=False) 
            
            if ch.startswith('D'):
                p.setYRange(-0.2, 1.2, padding=0)
            else:
                p.setYRange(0, 3.5, padding=0)
                # --- [UPDATE CẬP NHẬT TOOLTIP ĐIỆN ÁP] ---
                # Tạo một TextItem gắn vào kênh A0 để hiển thị điện áp
                self.a0_voltage_label = pg.TextItem(text="", color='#4682B4', fill='#FFFFFF', anchor=(0, 1))
                p.addItem(self.a0_voltage_label)
                # -----------------------------------------

            if i == 0:
                p0 = p
                p.showAxis('top')    
                p.hideAxis('bottom')
                p.setXRange(0, 100) 
            else:
                p.showAxis('bottom') 
                p.getAxis('bottom').setStyle(showValues=False) 
                p.getAxis('bottom').setHeight(0)               
                p.setXLink(p0)       

            p.showGrid(x=True, y=False, alpha=0.3)

            if i % 2 == 0 and ch.startswith('D'):
                p.getViewBox().setBackgroundColor('#F7F7F7')

            # [UPDATE] Khởi tạo sẵn đường nét để xóa/vẽ data
            curve = p.plot(pen=pg.mkPen(color=colors[ch], width=1.5))
            self.curves[ch] = curve

            v_line = pg.InfiniteLine(angle=90, movable=False, pen=pg.mkPen(color='#444444', width=1, style=Qt.DashLine))
            p.addItem(v_line)
            self.v_lines.append(v_line)

            # --- KHỞI TẠO CÔNG CỤ ĐO CHO TỪNG KÊNH ĐỘC LẬP ---
            region = pg.LinearRegionItem(values=[0, 0], movable=True)
            region.setBrush(QColor(255, 215, 0, 50)) 
            region.hide()
            p.addItem(region)
            self.measure_regions[ch] = region
            
            txt = pg.TextItem(text="", color='#000000', fill='#FFD700', anchor=(0.5, 0))
            txt.hide()
            p.addItem(txt)
            self.measure_texts[ch] = txt
            
            region.sigRegionChanged.connect(self.update_measurement)

            self.plots[ch] = p
            self.plot_widget.nextRow()
            
        self.proxy = pg.SignalProxy(self.plot_widget.scene().sigMouseMoved, rateLimit=60, slot=self.mouseMoved)

        # Bắt sự kiện Click chuột lên biểu đồ
        self.plot_widget.scene().sigMouseClicked.connect(self.on_mouse_clicked)
        # ------------------------------------------

        return self.plot_widget

    # ==================================================
    # SỰ KIỆN CLICK CHUỘT (TẠO/XÓA VÙNG ĐO)
    # ==================================================
    def on_mouse_clicked(self, evt):
        pos = evt.scenePos()
        for ch, p in self.plots.items():
            if p.sceneBoundingRect().contains(pos):
                mousePoint = p.vb.mapSceneToView(pos)
                x_val = mousePoint.x()

                region = self.measure_regions[ch]
                txt = self.measure_texts[ch]

                # Nếu Double-Click: Mở/Tắt vùng đo tại kênh ĐƯỢC CLICK
                if evt.double():
                    if region.isVisible():
                        region.hide()
                        txt.hide()
                    else:
                        # Mở rộng vùng đo bằng 10% khung hình hiện tại
                        view_range = p.viewRange()[0]
                        w = (view_range[1] - view_range[0]) * 0.1 
                        region.setRegion([x_val - w/2, x_val + w/2])
                        region.show()
                        txt.show()
                        self.update_measurement()
                        
                # Nếu Single-Click chuột trái: Kiểm tra xem có click ra ngoài vùng đo để Hủy không
                elif evt.button() == Qt.LeftButton:
                    if region.isVisible():
                        r_min, r_max = region.getRegion()
                        if x_val < r_min or x_val > r_max:
                            region.hide()
                            txt.hide()
                break

    # ==================================================
    # HÀM TÍNH TOÁN TẦN SỐ VÀ DUTY CYCLE
    # ==================================================
    def update_measurement(self):
        # Lặp qua tính toán cho TẤT CẢ các kênh
        for ch, p in self.plots.items():
            region = self.measure_regions.get(ch)
            txt_item = self.measure_texts.get(ch)

            # Bỏ qua nếu kênh này không bật thước đo
            if not region or not txt_item or not region.isVisible():
                continue

            minX, maxX = region.getRegion()
            if minX >= maxX: continue
            
            # Quy đổi thời gian (minX, maxX) ra Index của mảng Data
            start_idx = max(0, int(minX * self.current_sample_rate))
            end_idx = min(self.current_idx, int(maxX * self.current_sample_rate))
            text_x_pos = minX + (maxX - minX) / 2

            arr = getattr(self, f"{ch.lower()}_arr", None)
            
            # Canh chỉnh vị trí nhãn: D0-D6 để ở 1.1, A0 để ở 3.1
            y_pos = 1.1 if ch.startswith('D') else 3.1
            txt_item.setPos(text_x_pos, y_pos)

            # CHỐNG LƯU DỮ LIỆU CŨ: Ép reset các thông số về mặc định mỗi lần cập nhật
            duty_cycle_val = None 
            vpp_val = None
            freq = 0
            
            # Kiểm tra mảng data hợp lệ
            if arr is None or len(arr) == 0 or self.current_idx == 0 or (end_idx - start_idx < 2):
                if ch == 'A0':
                    txt_item.setText(" Freq: 0.00 Hz | Vpp: -- V ")
                else:
                    txt_item.setText(" Freq: 0.00 Hz | Duty: -- % ")
                continue
                
            data_slice = arr[start_idx:end_idx]
            
            # --- THUẬT TOÁN ĐO TẦN SỐ CHỐNG NHIỄU (HYSTERESIS) ---
            if ch == 'A0':
                _min = np.min(data_slice)
                _max = np.max(data_slice)
                vpp_val = _max - _min
                
                # Khử nhiễu: Nếu sóng quá phẳng (biên độ < 0.1V), coi như không có dao động
                if _max - _min < 0.1:
                    edge_indices = []
                else:
                    # Áp dụng Hysteresis để loại bỏ nhiễu răng cưa của DAC
                    threshold_high = _min + (_max - _min) * 0.6
                    threshold_low  = _min + (_max - _min) * 0.4
                    
                    states = np.zeros_like(data_slice, dtype=np.int8)
                    states[data_slice > threshold_high] = 1
                    states[data_slice < threshold_low] = -1
                    
                    valid_states_idx = np.where(states != 0)[0]
                    if len(valid_states_idx) > 0:
                        valid_states = states[valid_states_idx]
                        transitions = np.diff(valid_states) > 0 # Tìm cạnh lên (-1 -> 1)
                        edge_indices = valid_states_idx[:-1][transitions]
                    else:
                        edge_indices = []
            else:
                # Digital hoàn hảo không có nhiễu, tìm cạnh lên (0 chuyển sang 1)
                edge_indices = np.where(np.diff(data_slice) > 0)[0] + 1
                
                # TÍNH TOÁN DUTY CYCLE CHUẨN XÁC DỰA TRÊN CHU KỲ TRỌN VẸN
                if len(edge_indices) >= 2:
                    start_cycle = edge_indices[0]
                    end_cycle = edge_indices[-1]
                    
                    perfect_cycles_data = data_slice[start_cycle:end_cycle]
                    
                    high_samples = np.sum(perfect_cycles_data == 1)
                    total_samples = len(perfect_cycles_data)
                    
                    duty_cycle_val = (high_samples / total_samples) * 100.0 if total_samples > 0 else 0.0

            # ==========================================================
            # TÍNH TOÁN TẦN SỐ
            # ==========================================================
            if len(edge_indices) >= 2:
                num_cycles = len(edge_indices) - 1
                actual_time_span = (edge_indices[-1] - edge_indices[0]) / self.current_sample_rate
                freq = num_cycles / actual_time_span if actual_time_span > 0 else 0
            else:
                freq = 0
                
            # Định dạng chuỗi hiển thị Tần số
            if freq >= 1000000:
                freq_str = f"{freq/1000000:.2f} MHz"
            elif freq >= 1000:
                freq_str = f"{freq/1000:.2f} kHz"
            else:
                freq_str = f"{freq:.2f} Hz"
                
            # Cập nhật Nội dung Text dựa theo từng kênh (Analog vs Digital)
            if ch == 'A0':
                if vpp_val is not None:
                    txt_item.setText(f" Freq: {freq_str} | Vpp: {vpp_val:.2f} V ")
                else:
                    txt_item.setText(f" Freq: {freq_str} | Vpp: -- V ")
            else:
                if duty_cycle_val is not None:
                    txt_item.setText(f" Freq: {freq_str} | Duty: {duty_cycle_val:.1f}% ")
                else:
                    txt_item.setText(f" Freq: {freq_str} | Duty: -- % ")

    # ==================================================
    # SỰ KIỆN RÊ CHUỘT CROSSHAIR 
    # ==================================================
    def mouseMoved(self, evt):
        pos = evt[0]  
        for ch, p in self.plots.items():
            if p.sceneBoundingRect().contains(pos):
                mousePoint = p.vb.mapSceneToView(pos)
                mouse_x = mousePoint.x()
                
                # Cập nhật vị trí đường nét đứt cho tất cả các kênh
                for v_line in self.v_lines:
                    v_line.setPos(mouse_x)
                
                # --- [UPDATE CẬP NHẬT TOOLTIP ĐIỆN ÁP] ---
                # Nếu đã có dữ liệu trong mảng A0
                if hasattr(self, 'a0_arr') and len(self.a0_arr) > 0 and self.current_idx > 0:
                    # Tính toán index của mảng dựa trên tọa độ X (thời gian) và Sample Rate
                    idx = int(mouse_x * self.current_sample_rate)
                    
                    # Ràng buộc index nằm trong phạm vi dữ liệu đã thu thập
                    if 0 <= idx < self.current_idx:
                        voltage = self.a0_arr[idx]
                        self.a0_voltage_label.setText(f"{voltage:.2f} V")
                        # Gắn label nằm ngay cạnh đường gióng (dịch lên một chút so với đáy đồ thị)
                        self.a0_voltage_label.setPos(mouse_x, 0.2)
                    else:
                        self.a0_voltage_label.setText("") # Ẩn chữ nếu trỏ ra ngoài vùng có dữ liệu
                # -----------------------------------------
                break 

    # ==================================================
    # TIẾP NHẬN VÀ XỬ LÝ DỮ LIỆU TỐI ƯU HÓA (O(N))
    # ==================================================
    def process_raw_data(self, data: bytes):
        if self.is_running:
            self.raw_buffer.extend(data)

    def update_plots(self):
        # 1. Xác định xem có CẦN RENDER TOÀN BỘ KHÔNG (khi đang ở trạng thái Dừng)
        needs_final_render = (not self.is_running) and getattr(self, '_has_rendered_full', False) is False

        # Nếu không có data mới VÀ cũng không yêu cầu render cuối cùng thì mới thoát sớm để đỡ tốn CPU
        if len(self.raw_buffer) == 0 and len(self.leftover_byte) == 0:
            if not needs_final_render:
                return

        # Nối data cũ bị lẻ nhịp trước vào đầu chuỗi mới
        chunk = self.leftover_byte + self.raw_buffer[:]
        self.raw_buffer.clear()

        # Kiểm tra xem Pico đã gửi cờ '+' báo hoàn thành toàn bộ tác vụ chưa
        pico_finished = b'+' in chunk

        if len(chunk) > 0:
            valid_data = np.frombuffer(chunk, dtype=np.uint8)
            valid_data = valid_data[valid_data >= 0x80] # Loại rác

            if len(valid_data) % 2 != 0:
                self.leftover_byte = bytearray([valid_data[-1]])
                valid_data = valid_data[:-1]
            else:
                self.leftover_byte.clear()

            digital_bytes = valid_data[0::2]
            analog_bytes = valid_data[1::2]
            n_samples = len(digital_bytes)

            if n_samples > 0:
                if self.current_idx + n_samples > self.expected_samples:
                    n_samples = self.expected_samples - self.current_idx
                    digital_bytes = digital_bytes[:n_samples]
                    analog_bytes = analog_bytes[:n_samples]

                start_i = self.current_idx
                end_i = start_i + n_samples

                self.d0_arr[start_i:end_i] = digital_bytes & 1
                self.d1_arr[start_i:end_i] = (digital_bytes >> 1) & 1
                self.d2_arr[start_i:end_i] = (digital_bytes >> 2) & 1
                self.d3_arr[start_i:end_i] = (digital_bytes >> 3) & 1
                self.d4_arr[start_i:end_i] = (digital_bytes >> 4) & 1
                self.d5_arr[start_i:end_i] = (digital_bytes >> 5) & 1
                self.d6_arr[start_i:end_i] = (digital_bytes >> 6) & 1
                self.a0_arr[start_i:end_i] = (analog_bytes & 0x7F) * (3.3 / 127.0)

                self.current_idx = end_i

        # Lấy mốc index hiện tại để dùng cho việc vẽ (bảo vệ trường hợp end_i ở trên không được tạo)
        end_i = self.current_idx

        # ==========================================
        # KIỂM TRA HOÀN THÀNH HOẶC ĐỦ MẪU
        # ==========================================
        if (self.current_idx >= self.expected_samples or pico_finished) and self.is_running:
            self.toggle_start_stop()
            # Trả về luôn vì toggle_start_stop sẽ set is_running = False và gọi lại update_plots một lần nữa
            return 

        # ==========================================
        # HIỂN THỊ ĐỒ THỊ (RENDER LOGIC)
        # ==========================================
        current_t = time.time()
        if not hasattr(self, 'last_plot_time'):
            self.last_plot_time = 0
            
        if not hasattr(self, '_has_rendered_full'):
            self._has_rendered_full = False

        if self.is_running:
            # ---------------------------------------------------------
            # CHẾ ĐỘ ĐANG CHẠY (LIVE): Chỉ render nối đuôi (sliding window)
            # ---------------------------------------------------------
            if (current_t - self.last_plot_time >= 0.3) and end_i > 0:
                view_start = max(0, end_i - 2500)
                time_view = self.time_arr[view_start:end_i]

                self.curves['D0'].setData(time_view, self.d0_arr[view_start:end_i])
                self.curves['D1'].setData(time_view, self.d1_arr[view_start:end_i])
                self.curves['D2'].setData(time_view, self.d2_arr[view_start:end_i])
                self.curves['D3'].setData(time_view, self.d3_arr[view_start:end_i])
                self.curves['D4'].setData(time_view, self.d4_arr[view_start:end_i])
                self.curves['D5'].setData(time_view, self.d5_arr[view_start:end_i])
                self.curves['D6'].setData(time_view, self.d6_arr[view_start:end_i])
                self.curves['A0'].setData(time_view, self.a0_arr[view_start:end_i])

                # Tự động trượt thanh ngắm
                self.plots['D0'].setXRange(self.time_arr[view_start], self.time_arr[end_i - 1], padding=0)
                
                self.last_plot_time = current_t

        else:
            # ---------------------------------------------------------
            # CHẾ ĐỘ DỪNG (STOP): Render 1 lần toàn cảnh dữ liệu thu được
            # ---------------------------------------------------------
            if not self._has_rendered_full and end_i > 0:
                
                time_view = self.time_arr[0:end_i]
                self.curves['D0'].setData(time_view, self.d0_arr[0:end_i])
                self.curves['D1'].setData(time_view, self.d1_arr[0:end_i])
                self.curves['D2'].setData(time_view, self.d2_arr[0:end_i])
                self.curves['D3'].setData(time_view, self.d3_arr[0:end_i])
                self.curves['D4'].setData(time_view, self.d4_arr[0:end_i])
                self.curves['D5'].setData(time_view, self.d5_arr[0:end_i])
                self.curves['D6'].setData(time_view, self.d6_arr[0:end_i])
                self.curves['A0'].setData(time_view, self.a0_arr[0:end_i])

                # Xem toàn cảnh từ 0 đến kết thúc
                self.plots['D0'].setXRange(0, self.time_arr[end_i - 1], padding=0)
                
                # Khóa cờ lại, để các hàm cuộn/zoom chuột tự do mà không bị vẽ lại
                self._has_rendered_full = True

                # Cập nhật các công cụ đo tần số nếu đang bật
                self.update_measurement()

    # ==================================================
    # HÀM HỖ TRỢ XỬ LÝ LỆNH START/STOP
    # ==================================================
    def parse_number_value(self, text, multipliers):
        """Chuyển đổi text combobox ('1 M', '100 kHz') thành số thực tế"""
        text = text.replace(" ", "").upper()
        for suffix, mult in multipliers.items():
            if suffix in text:
                return int(float(text.replace(suffix, "")) * mult)
        return int(text)

    def reset_plots_to_zero(self):
        """Xóa hết dữ liệu cũ và ép trục thời gian về lại mốc 0"""
        for ch in self.curves:
            self.curves[ch].setData([], []) 
        
        if 'D0' in self.plots:
            self.plots['D0'].setXRange(0, 100, padding=0)
            
        # Reset text điện áp
        if hasattr(self, 'a0_voltage_label'):
            self.a0_voltage_label.setText("")
            
        # Reset (Ẩn) công cụ đo của tất cả các kênh khi Start lại
        if hasattr(self, 'measure_regions'):
            for ch in self.measure_regions:
                self.measure_regions[ch].hide()
                self.measure_texts[ch].hide()

    # ==================================================
    # SỰ KIỆN NÚT BẤM (GỬI LỆNH)
    # ==================================================
    def toggle_start_stop(self):
        if getattr(self.uart_logic, 'ser', None) is None or not self.uart_logic.ser.is_open:
            return

        if not self.is_running:
            self.is_running = True
            self.btn_start.setText(" STOP")
            self.btn_start.setIcon(self.create_led_icon("#4CAF50")) 

            rate = self.parse_number_value(self.cb_rate.currentText(), {"KHZ": 1000, "MHZ": 1000000})
            samples = self.parse_number_value(self.cb_samples.currentText(), {"K": 1000, "M": 1000000, "G": 1000000000})

            # RESET BIẾN TRẠNG THÁI
            self.current_sample_rate = rate
            self.expected_samples = samples
            self.raw_buffer.clear()
            self.leftover_byte.clear()
            self.current_idx = 0
            self.last_plot_time = 0 # Trả lại mốc thời gian để sẵn sàng vẽ khung đầu tiên
            self._has_rendered_full = False
            self.reset_plots_to_zero()
            
            # ------------------------------------------------
            # TIỀN CẤP PHÁT BỘ NHỚ (NGUYÊN LÝ VÀNG CHỐNG LAG)
            # ------------------------------------------------
            self.d0_arr = np.zeros(samples, dtype=np.int8)
            self.d1_arr = np.zeros(samples, dtype=np.int8)
            self.d2_arr = np.zeros(samples, dtype=np.int8)
            self.d3_arr = np.zeros(samples, dtype=np.int8)
            self.d4_arr = np.zeros(samples, dtype=np.int8)
            self.d5_arr = np.zeros(samples, dtype=np.int8)
            self.d6_arr = np.zeros(samples, dtype=np.int8)
            self.a0_arr = np.zeros(samples, dtype=np.float32)
            
            dt = 1.0 / rate
            self.time_arr = np.arange(samples) * dt
            
            self.plot_timer.start(30) 

            try:
                # 1. Reset state
                self.uart_logic.ser.write(b"*")
                time.sleep(0.01)
                
                # 2. Setup kênh
                self.uart_logic.ser.write(b"A10\n")
                time.sleep(0.01)
                self.uart_logic.ser.write(b"D10\n")
                time.sleep(0.01)
                self.uart_logic.ser.write(b"D11\n")
                time.sleep(0.01)
                self.uart_logic.ser.write(b"D12\n")
                time.sleep(0.01)
                self.uart_logic.ser.write(b"D13\n")
                time.sleep(0.01)
                self.uart_logic.ser.write(b"D14\n")
                time.sleep(0.01)
                self.uart_logic.ser.write(b"D15\n")
                time.sleep(0.01)
                self.uart_logic.ser.write(b"D16\n")
                time.sleep(0.01)
                
                # 3. Setup tham số
                self.uart_logic.ser.write(f"R{rate}\n".encode('ascii'))
                time.sleep(0.01)
                self.uart_logic.ser.write(f"L{samples}\n".encode('ascii'))
                time.sleep(0.01)
                
                # Để test tạm thời tránh Python bị kẹt `readline()`, 
                # bạn phải XÓA/FLUSH buffer RX cũ trước khi ra lệnh START
                self.uart_logic.ser.reset_input_buffer()
                
                # 4. Gửi lệnh chốt (Bắn Data)
                self.uart_logic.ser.write(b"F\n")

                print(f"[LOGIC] Bắt đầu: Setup {samples} samples @ {rate} Hz")
            except Exception as e:
                print(f"[LOGIC] Lỗi gửi lệnh START: {e}")
                self.is_running = False
                self.btn_start.setText(" START")
                self.btn_start.setIcon(self.create_led_icon("gray"))
                self.plot_timer.stop()

        else:
            self.is_running = False
            self.btn_start.setText(" START")
            self.btn_start.setIcon(self.create_led_icon("gray"))
            
            self.plot_timer.stop()
            self.update_plots() # Chạy update_plots lần cuối để render toàn cảnh
            
            try:
                self.uart_logic.ser.write(b"+")
            except Exception as e:
                print(f"[LOGIC] Lỗi gửi lệnh STOP: {e}")

