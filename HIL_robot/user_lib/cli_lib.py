import serial
import time

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
        time.sleep(1)

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