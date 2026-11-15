#include "i2c_slave_emu.h"

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/sync.h"

// Defined in pico_sdk_sigrok.c: fast write to the USB CDC.
extern void my_stdio_usb_out_chars(const char *buf, int length);

#define EMU_I2C      i2c0
#define EMU_IRQ      I2C0_IRQ
#define EMU_SDA_PIN  0   // i2c0 SDA
#define EMU_SCL_PIN  1   // i2c0 SCL
#define EMU_BAUD     400000   // selects fast mode; a slave also works with a 100 kHz master

// Event log: one uint16_t per event. Low 8 bits = data byte.
#define EV_START 0x100
#define EV_STOP  0x200
#define EV_WR    0x400   // byte received from the master (master write)
#define EV_RD    0x800   // byte sent to the master (master read)

#define LOG_SIZE 1024    // must be a power of two (2 KB of RAM)

static volatile uint16_t log_buf[LOG_SIZE];
static volatile uint32_t log_head;   // written by the IRQ
static volatile uint32_t log_tail;   // written by the main loop
static volatile uint32_t dropped;
static bool    enabled;
static uint8_t slave_addr;
static bool    irq_registered;

// Emulated device: I2C_EMU_REG_COUNT 8-bit registers behind an 8-bit register pointer.
static volatile uint8_t regs[I2C_EMU_REG_COUNT];
static volatile uint8_t reg_ptr;
static volatile bool    expect_reg;   // next written byte is the register address

// ---- DS3231 RTC mode (It1 / It0) ----------------------------------------------------------
// The time registers 0x00..0x06 (BCD, 24 h mode) advance by one second, like a running DS3231.
// While an I2C transaction is in progress (START..STOP) the time is frozen, so a burst read
// never sees a half-updated time (the real chip latches its user buffer the same way).
// Writing the seconds register restarts the 1 s countdown (also like the real chip).
#define RTC_TICK_US        1000000u
#define RTC_XFER_STUCK_US  50000u   // a transaction without STOP for this long does not block ticking

static volatile bool     rtc_on;
static volatile bool     in_xfer;
static volatile uint64_t xfer_start_us;
static volatile uint64_t next_tick_us;
static repeating_timer_t rtc_timer;
static bool              rtc_timer_running;

static inline uint8_t bcd2bin(uint8_t v) { return (uint8_t)((v >> 4) * 10 + (v & 0x0F)); }
static inline uint8_t bin2bcd(uint8_t v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }

static uint8_t days_in_month(uint8_t month, uint8_t year)
{
	static const uint8_t dim[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	if (month < 1 || month > 12) {
		return 31;
	}
	if (month == 2 && (year % 4) == 0) {   // years 2000..2099
		return 29;
	}
	return dim[month - 1];
}

// Advance the time registers by one second.
static void rtc_tick(void)
{
	uint8_t s = bcd2bin(regs[0] & 0x7F) + 1;
	if (s < 60) { regs[0] = bin2bcd(s); return; }
	regs[0] = 0x00;

	uint8_t m = bcd2bin(regs[1] & 0x7F) + 1;
	if (m < 60) { regs[1] = bin2bcd(m); return; }
	regs[1] = 0x00;

	uint8_t h = bcd2bin(regs[2] & 0x3F) + 1;
	if (h < 24) { regs[2] = bin2bcd(h); return; }
	regs[2] = 0x00;

	// new day
	uint8_t dow = regs[3] & 0x07;
	regs[3] = (dow == 0 || dow >= 7) ? 1 : dow + 1;

	uint8_t century = regs[5] & 0x80;
	uint8_t mon = bcd2bin(regs[5] & 0x1F);
	uint8_t yr  = bcd2bin(regs[6]);
	uint8_t d   = bcd2bin(regs[4] & 0x3F) + 1;
	if (d <= days_in_month(mon, yr)) { regs[4] = bin2bcd(d); return; }
	regs[4] = 0x01;

	if (++mon <= 12) { regs[5] = century | bin2bcd(mon); return; }
	regs[5] = century | 0x01;

	if (++yr <= 99) { regs[6] = bin2bcd(yr); return; }
	regs[6] = 0x00;
	regs[5] = (century ^ 0x80) | 0x01;   // 2099 -> 2100: the century bit toggles
}

// Apply every second that has elapsed. Called with the I2C / timer IRQs not able to preempt.
static void rtc_catch_up(void)
{
	uint64_t now = time_us_64();
	int n = 0;
	while (now >= next_tick_us && n < 100) {
		rtc_tick();
		next_tick_us += RTC_TICK_US;
		n++;
	}
	if (now >= next_tick_us) {   // far behind (should not happen): resync
		next_tick_us = now + RTC_TICK_US;
	}
}

static bool rtc_timer_cb(repeating_timer_t *t)
{
	(void)t;
	if (rtc_on && (!in_xfer || (time_us_64() - xfer_start_us) > RTC_XFER_STUCK_US)) {
		rtc_catch_up();
	}
	return true;
}

static inline void push(uint16_t ev)
{
	uint32_t h = log_head;
	if ((h - log_tail) >= LOG_SIZE) {
		dropped++;
		return;
	}
	log_buf[h & (LOG_SIZE - 1)] = ev;
	log_head = h + 1;
}

static inline uint8_t reg_read_next(void)
{
	uint8_t v = (reg_ptr < I2C_EMU_REG_COUNT) ? regs[reg_ptr] : 0xFF;   // unmapped -> 0xFF
	reg_ptr++;
	return v;
}

static inline void reg_write_next(uint8_t v)
{
	if (reg_ptr < I2C_EMU_REG_COUNT) {
		if (rtc_on && reg_ptr == 0) {
			v &= 0x7F;                                 // DS3231: bit 7 of seconds reads 0
			next_tick_us = time_us_64() + RTC_TICK_US; // writing seconds restarts the countdown
		}
		regs[reg_ptr] = v;   // unmapped registers ignore writes
	}
	reg_ptr++;
}

static void i2c_emu_irq(void)
{
	i2c_hw_t *hw = i2c_get_hw(EMU_I2C);
	uint32_t st = hw->intr_stat;

	// A master NACK after the last byte of a read raises TX_ABRT: normal, just clear it.
	if (st & I2C_IC_INTR_STAT_R_TX_ABRT_BITS) {
		(void)hw->clr_tx_abrt;
	}
	if (st & I2C_IC_INTR_STAT_R_START_DET_BITS) {   // START or repeated START
		(void)hw->clr_start_det;
		expect_reg = true;
		if (rtc_on) {
			rtc_catch_up();   // up-to-date time, then frozen until STOP
			in_xfer = true;
			xfer_start_us = time_us_64();
		}
		push(EV_START);
	}
	// The hardware has already ACKed the address and each of these data bytes.
	if (st & I2C_IC_INTR_STAT_R_RX_FULL_BITS) {
		while (hw->status & I2C_IC_STATUS_RFNE_BITS) {
			uint32_t dc = hw->data_cmd;
			uint8_t b = (uint8_t)dc;
			// First byte after the address phase = register address (pointer), like a real sensor.
			if (expect_reg || (dc & I2C_IC_DATA_CMD_FIRST_DATA_BYTE_BITS)) {
				reg_ptr = b;
				expect_reg = false;
			} else {
				reg_write_next(b);
			}
			push(EV_WR | b);
		}
	}
	if (st & I2C_IC_INTR_STAT_R_RD_REQ_BITS) {
		(void)hw->clr_rd_req;
		uint8_t b = reg_read_next();
		hw->data_cmd = b;
		push(EV_RD | b);
	}
	if (st & I2C_IC_INTR_STAT_R_STOP_DET_BITS) {
		(void)hw->clr_stop_det;
		push(EV_STOP);
		if (rtc_on) {
			in_xfer = false;
			rtc_catch_up();
		}
	}
}

bool i2c_emu_enable(uint8_t addr7)
{
	if (addr7 < 0x08 || addr7 > 0x77) {
		return false;
	}
	i2c_emu_disable();
	i2c_emu_clear();
	reg_ptr = 0;
	expect_reg = true;

	i2c_init(EMU_I2C, EMU_BAUD);
	gpio_set_function(EMU_SDA_PIN, GPIO_FUNC_I2C);
	gpio_set_function(EMU_SCL_PIN, GPIO_FUNC_I2C);
	i2c_set_slave_mode(EMU_I2C, true, addr7);

	i2c_hw_t *hw = i2c_get_hw(EMU_I2C);
	hw->rx_tl = 0;   // RX_FULL as soon as one byte arrives
	hw->intr_mask = I2C_IC_INTR_MASK_M_RX_FULL_BITS
	              | I2C_IC_INTR_MASK_M_RD_REQ_BITS
	              | I2C_IC_INTR_MASK_M_TX_ABRT_BITS
	              | I2C_IC_INTR_MASK_M_STOP_DET_BITS
	              | I2C_IC_INTR_MASK_M_START_DET_BITS;

	if (!irq_registered) {
		irq_set_exclusive_handler(EMU_IRQ, i2c_emu_irq);
		irq_registered = true;
	}
	irq_set_enabled(EMU_IRQ, true);

	slave_addr = addr7;
	enabled = true;
	return true;
}

void i2c_emu_disable(void)
{
	if (!enabled) {
		return;
	}
	irq_set_enabled(EMU_IRQ, false);
	i2c_deinit(EMU_I2C);
	gpio_init(EMU_SDA_PIN);   // back to plain input
	gpio_init(EMU_SCL_PIN);
	enabled = false;
}

void i2c_emu_clear(void)
{
	log_tail = log_head;
	dropped = 0;
}

bool i2c_emu_set_reg(uint8_t reg, uint8_t value)
{
	if (reg >= I2C_EMU_REG_COUNT) {
		return false;
	}
	uint32_t irq = save_and_disable_interrupts();
	if (rtc_on && reg == 0) {
		value &= 0x7F;
		next_tick_us = time_us_64() + RTC_TICK_US;
	}
	regs[reg] = value;
	restore_interrupts(irq);
	return true;
}

int i2c_emu_get_reg(uint8_t reg)
{
	return (reg < I2C_EMU_REG_COUNT) ? regs[reg] : -1;
}

void i2c_emu_reset_regs(void)
{
	uint32_t irq = save_and_disable_interrupts();
	for (int i = 0; i < I2C_EMU_REG_COUNT; i++) {
		regs[i] = 0;
	}
	if (rtc_on) {
		// DS3231 power-on state: 2000-01-01 00:00:00, day 1, control 0x1C, status OSF + EN32kHz
		regs[0x03] = 0x01;
		regs[0x04] = 0x01;
		regs[0x05] = 0x01;
		regs[0x0E] = 0x1C;
		regs[0x0F] = 0x88;
		next_tick_us = time_us_64() + RTC_TICK_US;
	}
	restore_interrupts(irq);
}

void i2c_emu_rtc_enable(bool on)
{
	uint32_t irq = save_and_disable_interrupts();
	rtc_on = on;
	in_xfer = false;
	next_tick_us = time_us_64() + RTC_TICK_US;
	restore_interrupts(irq);

	if (on && !rtc_timer_running) {
		rtc_timer_running = add_repeating_timer_ms(10, rtc_timer_cb, NULL, &rtc_timer);
	} else if (!on && rtc_timer_running) {
		cancel_repeating_timer(&rtc_timer);
		rtc_timer_running = false;
	}
}

bool i2c_emu_rtc_is_on(void)
{
	return rtc_on;
}

void i2c_emu_status(char *out, int out_len)
{
	if (enabled) {
		// keep it short: sr_device rspstr is only 20 chars, e.g. "I2C ON 42 E6 D0"
		snprintf(out, out_len, "I2C ON %02X E%u D%u\n", slave_addr,
		         (unsigned)(log_head - log_tail), (unsigned)dropped);
	} else {
		snprintf(out, out_len, "I2C OFF\n");
	}
}

// Register map, one line: "REGS 5A 00 12 34 ..." (I2C_EMU_REG_COUNT bytes)
void i2c_emu_dump_regs(void)
{
	char line[8 + 3 * I2C_EMU_REG_COUNT + 2];
	uint8_t snap[I2C_EMU_REG_COUNT];
	uint32_t irq = save_and_disable_interrupts();
	if (rtc_on && !in_xfer) {
		rtc_catch_up();
	}
	for (int i = 0; i < I2C_EMU_REG_COUNT; i++) {
		snap[i] = regs[i];
	}
	restore_interrupts(irq);
	int n = snprintf(line, sizeof(line), "REGS");
	for (int i = 0; i < I2C_EMU_REG_COUNT; i++) {
		n += snprintf(line + n, sizeof(line) - n, " %02X", snap[i]);
	}
	n += snprintf(line + n, sizeof(line) - n, "\n");
	my_stdio_usb_out_chars(line, n);
}

// Text dump, I2C-sniffer style:  S W 00 00 01 02 P   /   S W 10 S R A0 A1 P
void i2c_emu_dump(void)
{
	char line[128];
	int n = 0;
	char dir = 0;

	while (log_tail != log_head) {
		uint16_t ev = log_buf[log_tail & (LOG_SIZE - 1)];
		log_tail = log_tail + 1;

		if (ev & EV_START) {
			n += snprintf(line + n, sizeof(line) - n, "S ");
			dir = 0;
		} else if (ev & EV_STOP) {
			n += snprintf(line + n, sizeof(line) - n, "P\n");
			dir = 0;
		} else {
			char want = (ev & EV_WR) ? 'W' : 'R';
			if (dir != want) {
				n += snprintf(line + n, sizeof(line) - n, "%c ", want);
				dir = want;
			}
			n += snprintf(line + n, sizeof(line) - n, "%02X ", (unsigned)(ev & 0xFF));
		}
		// flush before the buffer can overflow
		if (n > (int)sizeof(line) - 24) {
			my_stdio_usb_out_chars(line, n);
			n = 0;
		}
	}
	if (n > 0) {
		my_stdio_usb_out_chars(line, n);
	}
	char tail[32];
	int m = snprintf(tail, sizeof(tail), "\nEND DROP %u\n", (unsigned)dropped);
	my_stdio_usb_out_chars(tail, m);
}
