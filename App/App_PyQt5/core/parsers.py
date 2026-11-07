# File: core/parsers.py

class SPIParser:
    def __init__(self, emit_cb, log_cb, clear_cb, pc_log_cb, test_complete_cb=None, highlight_error_cb=None):
        self.emit_cb = emit_cb
        self.log_cb = log_cb
        self.clear_cb = clear_cb
        self.pc_log_cb = pc_log_cb
        self.test_complete_cb = test_complete_cb
        self.highlight_error_cb = highlight_error_cb
        self.active = False
        self.expected = 0
        self.buffer = []
        self.wait_id = False 
        
        self.write_test_data = None  
        self.write_test_addr = 0
        self.write_test_size = 0
        self.write_test_pending = False  

    def feed(self, line):
        if line.startswith("w25q_read"):
            try:
                _, n = line.split()
                self.expected = int(n)
                self.buffer.clear()
                self.active = True
                self.clear_cb() 
                return True
            except: return True
        elif line.startswith("w25q_write"):
            try:
                _, n = line.split()
                addr = int(n)
                self.prepare_write_test_data(addr)
                self.write_test_pending = True
                return True
            except: return True
        elif line.startswith("w25q_prepare_mem"):
            try:
                _, n = line.split()
                addr = int(n)
                self.prepare_write_test_data(addr)
                self.pc_log_cb(f"Memory prepared at 0x{addr:04X}, size={self.write_test_size}")
                return True
            except: return True
        elif line.startswith("w25q_ID"):
            self.wait_id = True
            return True   

        if self.active and line.startswith("Read"):
            return True
        
        if self.write_test_pending and "invalid length" in line.lower():
            self.pc_log_cb("FAIL (Invalid length)")
            self.write_test_pending = False
            if self.test_complete_cb: self.test_complete_cb("spi")
            return True
        
        if self.wait_id and line.startswith("W25Q ID:"):
            try:
                raw = line.split("0x")[1]
                flash_id = raw[-6:].upper()
                if flash_id == "EF4018":
                    self.pc_log_cb(f"PASS (ID = {flash_id})")
                else:
                    self.pc_log_cb(f"FAIL (ID = {flash_id})")
            except:
                self.pc_log_cb("Flash ID PARSE ERROR")

            self.wait_id = False
            if self.test_complete_cb: self.test_complete_cb("spi")
            return True

        if self.active:
            for p in line.split():
                if len(p) == 2:
                    try: self.buffer.append(int(p, 16))
                    except ValueError: pass

            if len(self.buffer) >= self.expected:
                for addr, val in enumerate(self.buffer[:1024]):
                    self.emit_cb(addr, val)

                if self.write_test_data:
                    self.compare_write_test()
                    self.write_test_pending = False
                
                self.active = False
                if self.test_complete_cb:
                    self.test_complete_cb("spi")
            return True
        return False
    
    def prepare_write_test_data(self, addr):
        self.write_test_data = [(i % 256) for i in range(addr)]
        self.write_test_addr = addr
        self.write_test_size = addr
    
    def compare_write_test(self):
        if not self.write_test_data or len(self.buffer) != len(self.write_test_data):
            self.pc_log_cb("FAIL (size mismatch)")
            return
        mismatches = []
        for i, (expected, received) in enumerate(zip(self.write_test_data, self.buffer)):
            if expected != received:
                mismatches.append((i, expected, received))
        
        if not mismatches:
            self.pc_log_cb("PASS")
        else:
            if self.highlight_error_cb:
                for addr, exp, rcv in mismatches:
                    self.highlight_error_cb(addr)
            
            msg = f"FAIL ({len(mismatches)} mismatches)"
            for i, exp, rcv in mismatches[:5]:
                msg += f"\n  [0x{i:04X}] expected 0x{exp:02X}, got 0x{rcv:02X}"
            if len(mismatches) > 5:
                msg += f"\n  ... and {len(mismatches) - 5} more"
            self.pc_log_cb(msg)
    
    def reset(self):
        self.active = False
        self.expected = 0
        self.buffer = []
        self.wait_id = False
        self.write_test_data = None
        self.write_test_addr = 0
        self.write_test_size = 0
        self.write_test_pending = False

    def set_prepare_mem_size(self, size):
        self.prepare_write_test_data(size)

    def set_erase_test_data(self):
        self.write_test_data = [0xFF] * 1024
        self.write_test_addr = 0
        self.write_test_size = 1024

class I2CParser:
    def __init__(self, emit_cb, log_cb, clear_cb, test_complete_cb=None):
        self.emit_cb = emit_cb
        self.log_cb = log_cb
        self.clear_cb = clear_cb
        self.test_complete_cb = test_complete_cb
        self.active = False
        self.base_addr = 0
        self.expected = 0
        self.buffer = []

    def feed(self, line):
        if line.startswith("eeprom_read"):
            try:
                _, addr, n = line.split()
                self.base_addr = int(addr, 16)
                self.expected = int(n)
                self.buffer.clear()
                self.active = True
                self.clear_cb() 
                return True
            except: return True

        if self.active and "FAIL" in line.upper():
            self.log_cb("[I2C] EEPROM read FAILED")
            self.active = False
            if self.test_complete_cb: self.test_complete_cb("i2c")
            return True

        if self.active and line.startswith("EEPROM read"):
            return True

        if self.active:
            for p in line.split():
                try: self.buffer.append(int(p, 16))
                except: pass

            if len(self.buffer) >= self.expected:
                for i, val in enumerate(self.buffer):
                    addr = self.base_addr + i
                    self.emit_cb(addr, val)

                self.log_cb("[I2C] EEPROM read completed")
                self.active = False
                if self.test_complete_cb: self.test_complete_cb("i2c")
            return True
        return False

    def reset(self):
        self.active = False
        self.base_addr = 0
        self.expected = 0
        self.buffer = []

class UARTParser:
    def __init__(self, byte_cb, log_cb=None):
        self.byte_cb = byte_cb
        self.log_cb = log_cb

    def feed(self, line):
        parsed = False
        parts = line.strip().split()

        for p in parts:
            if len(p) == 2:
                try:
                    val = int(p, 16)
                    if 0 <= val <= 255:
                        self.byte_cb(val)
                        parsed = True
                except ValueError:
                    continue

        return parsed