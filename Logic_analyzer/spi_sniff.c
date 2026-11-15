#include "spi_sniff.h"

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pio.h"

// Defined in pico_sdk_sigrok.c: fast write to the USB CDC.
extern void my_stdio_usb_out_chars(const char *buf, int length);

#define SCK_PIN   10
#define MOSI_PIN  11
#define MISO_PIN  12
#define CS_PIN    13

// pio0 belongs to the logic analyzer capture, use pio1 here.
#define SNIFF_PIO    pio1
#define SNIFF_IRQ    PIO1_IRQ_0
#define SM_MOSI      0
#define SM_MISO      1
#define SM_REPLY     2   // drives MISO (pindirs) only while CS is low (Pico acting as the slave)
#define SM_TX        3   // device mode: shifts register data out on MISO, one bit per SCK

// Log entry: data = MOSI (bits 0..7) | MISO (bits 8..15) | EV_DATA, or a frame marker
#define EV_DATA  (1u << 18)
#define EV_START (1u << 16)
#define EV_STOP  (1u << 17)

#define LOG_SIZE 2048   // entries, power of two (8 KB of RAM). Keep small: the LA mallocs 476 KB at boot

static volatile uint32_t log_buf[LOG_SIZE];
static volatile uint32_t log_head;   // written by the IRQs
static volatile uint32_t log_tail;   // written by the main loop
static volatile uint32_t dropped;

static bool enabled;
static int  spi_mode;
static uint prog_offset;
static uint16_t prog_instr[3];
static pio_program_t prog = { .instructions = prog_instr, .length = 3, .origin = -1 };

// Reply program: hold MISO driven only while CS is asserted, high-Z otherwise.
static int  reply_level = -1;   // -1 = passive (MISO never driven), 0 = 0x00 bytes, 1 = 0xFF bytes
static bool reply_running;
static uint reply_offset;
static uint16_t reply_instr[4];
static pio_program_t reply_prog = { .instructions = reply_instr, .length = 4, .origin = -1 };

// Device mode: register-based SPI device, first byte = R/W bit (bit 7) + register address.
static bool dev_mode;
static bool tx_running;
static uint tx_offset;
static uint16_t tx_instr[3];
static pio_program_t tx_prog = { .instructions = tx_instr, .length = 3, .origin = -1 };
static volatile uint8_t regs[SPI_DEV_REG_COUNT];
static volatile uint32_t byte_idx;    // byte index inside the current CS frame
static volatile bool     dev_read;    // current frame is a read
static volatile uint8_t  reg_ptr;     // next register written by the master
static volatile uint8_t  tx_ptr;      // next register queued for MISO

static inline void __not_in_flash_func(push)(uint32_t ev)
{
	uint32_t h = log_head;
	if ((h - log_tail) >= LOG_SIZE) {
		dropped++;
		return;
	}
	log_buf[h & (LOG_SIZE - 1)] = ev;
	log_head = h + 1;
}

static inline uint8_t __not_in_flash_func(reg_value)(uint8_t r)
{
	return (r < SPI_DEV_REG_COUNT) ? regs[r] : 0xFF;   // unmapped registers read 0xFF
}

// Queue register bytes for MISO while there is room (MSB first: byte in the top 8 bits).
static inline void __not_in_flash_func(tx_top_up)(void)
{
	while (!pio_sm_is_tx_fifo_full(SNIFF_PIO, SM_TX)) {
		pio_sm_put(SNIFF_PIO, SM_TX, (uint32_t)reg_value(tx_ptr) << 24);
		tx_ptr++;
	}
}

// Device logic, called for every complete MOSI byte (in IRQ context).
static void __not_in_flash_func(dev_on_byte)(uint8_t mo)
{
	if (byte_idx == 0) {
		dev_read = (mo & 0x80) != 0;
		reg_ptr = mo & 0x7F;
		tx_ptr = reg_ptr;
		if (dev_read) {
			tx_top_up();   // data for the next bytes must be ready before the next SCK edge
		}
	} else if (dev_read) {
		tx_top_up();
	} else {
		if (reg_ptr < SPI_DEV_REG_COUNT) {
			regs[reg_ptr] = mo;   // unmapped registers ignore writes
		}
		reg_ptr++;
	}
	byte_idx++;
}

// Move complete MOSI/MISO byte pairs from the PIO FIFOs to the log.
static void __not_in_flash_func(drain)(void)
{
	while (!pio_sm_is_rx_fifo_empty(SNIFF_PIO, SM_MOSI) && !pio_sm_is_rx_fifo_empty(SNIFF_PIO, SM_MISO)) {
		uint32_t mo = pio_sm_get(SNIFF_PIO, SM_MOSI) & 0xFF;
		uint32_t mi = pio_sm_get(SNIFF_PIO, SM_MISO) & 0xFF;
		if (dev_mode) {
			dev_on_byte((uint8_t)mo);
		}
		push(EV_DATA | mo | (mi << 8));
	}
}

static void __not_in_flash_func(pio_irq_handler)(void)
{
	drain();
}

// Device mode: restart the MISO shifter and queue a dummy byte for the command byte of the next frame.
static void __not_in_flash_func(tx_restart)(void)
{
	if (!tx_running) {
		return;
	}
	pio_sm_set_enabled(SNIFF_PIO, SM_TX, false);
	pio_sm_clear_fifos(SNIFF_PIO, SM_TX);
	pio_sm_restart(SNIFF_PIO, SM_TX);   // empties the OSR
	pio_sm_exec(SNIFF_PIO, SM_TX, pio_encode_jmp(tx_offset));
	pio_sm_put(SNIFF_PIO, SM_TX, 0x00u << 24);   // MISO during the command byte
	pio_sm_set_enabled(SNIFF_PIO, SM_TX, true);
}

// Resynchronise the state machines on a byte boundary (bus idle, CS high).
static void restart_sms(void)
{
	const uint32_t mask = (1u << SM_MOSI) | (1u << SM_MISO);
	pio_set_sm_mask_enabled(SNIFF_PIO, mask, false);
	pio_sm_clear_fifos(SNIFF_PIO, SM_MOSI);
	pio_sm_clear_fifos(SNIFF_PIO, SM_MISO);
	pio_sm_restart(SNIFF_PIO, SM_MOSI);
	pio_sm_restart(SNIFF_PIO, SM_MISO);
	pio_sm_exec(SNIFF_PIO, SM_MOSI, pio_encode_jmp(prog_offset));
	pio_sm_exec(SNIFF_PIO, SM_MISO, pio_encode_jmp(prog_offset));
	pio_enable_sm_mask_in_sync(SNIFF_PIO, mask);
	tx_restart();
}

static void __not_in_flash_func(frame_start)(void)
{
	byte_idx = 0;
	push(EV_START);
}

static void __not_in_flash_func(cs_callback)(uint gpio, uint32_t events)
{
	if (gpio != CS_PIN || !enabled) {
		return;
	}
	bool fall = (events & GPIO_IRQ_EDGE_FALL) != 0;
	bool rise = (events & GPIO_IRQ_EDGE_RISE) != 0;

	if (fall && rise) {
		// Both edges latched before we got here: CS was deasserted for only a few ns/us
		// (the DUT driver raises and lowers CS back-to-back between commands).
		// The current CS level tells which edge came first.
		if (!gpio_get(CS_PIN)) {
			// rise then fall: previous frame ended, the next one has already started.
			drain();
			push(EV_STOP);
			tx_restart();     // the device needs a fresh MISO shifter for the new frame
			frame_start();    // no sniffer restart: the new frame may already be clocking
		} else {
			// fall then rise: a whole (very short) frame happened.
			frame_start();
			drain();
			restart_sms();
			push(EV_STOP);
		}
		return;
	}
	if (fall) {
		frame_start();
	}
	if (rise) {
		drain();            // all complete bytes of this frame
		restart_sms();      // drop a partial byte, if any
		push(EV_STOP);
	}
}

static void sm_setup(uint sm, uint data_pin)
{
	pio_sm_config c = pio_get_default_sm_config();
	sm_config_set_wrap(&c, prog_offset, prog_offset + prog.length - 1);
	sm_config_set_in_pins(&c, data_pin);
	sm_config_set_in_shift(&c, false, true, 8);   // shift left = MSB first, autopush every 8 bits
	sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
	sm_config_set_clkdiv(&c, 1.0f);
	pio_sm_init(SNIFF_PIO, sm, prog_offset, &c);
}

static void reply_stop(void)
{
	if (!reply_running) {
		return;
	}
	pio_sm_set_enabled(SNIFF_PIO, SM_REPLY, false);
	pio_sm_set_consecutive_pindirs(SNIFF_PIO, SM_REPLY, MISO_PIN, 1, false);   // release MISO
	pio_remove_program(SNIFF_PIO, &reply_prog, reply_offset);
	gpio_init(MISO_PIN);   // back to a plain input (still sampled by the sniffer)
	gpio_set_dir(MISO_PIN, GPIO_IN);
	reply_running = false;
}

// Drive MISO only while CS is low. `level` is the MISO value when nothing else sets it
// (constant reply); in device mode SM_TX overwrites the value bit by bit.
static void reply_start(int level)
{
	reply_stop();
	reply_instr[0] = pio_encode_wait_gpio(false, CS_PIN);   // CS asserted
	reply_instr[1] = pio_encode_set(pio_pindirs, 1);        // drive MISO
	reply_instr[2] = pio_encode_wait_gpio(true, CS_PIN);    // CS released
	reply_instr[3] = pio_encode_set(pio_pindirs, 0);        // high-Z again
	reply_offset = pio_add_program(SNIFF_PIO, &reply_prog);

	pio_sm_config c = pio_get_default_sm_config();
	sm_config_set_wrap(&c, reply_offset, reply_offset + reply_prog.length - 1);
	sm_config_set_set_pins(&c, MISO_PIN, 1);
	sm_config_set_clkdiv(&c, 1.0f);
	pio_sm_init(SNIFF_PIO, SM_REPLY, reply_offset, &c);

	pio_gpio_init(SNIFF_PIO, MISO_PIN);   // pio1 owns the pad output; input still readable by the sniffer
	pio_sm_set_pins_with_mask(SNIFF_PIO, SM_REPLY, (uint32_t)(level ? 1 : 0) << MISO_PIN, 1u << MISO_PIN);
	pio_sm_set_consecutive_pindirs(SNIFF_PIO, SM_REPLY, MISO_PIN, 1, false);   // start released
	pio_sm_set_enabled(SNIFF_PIO, SM_REPLY, true);
	reply_running = true;
}

static void tx_stop(void)
{
	if (!tx_running) {
		return;
	}
	pio_sm_set_enabled(SNIFF_PIO, SM_TX, false);
	pio_remove_program(SNIFF_PIO, &tx_prog, tx_offset);
	tx_running = false;
}

// MISO shifter for CPHA = 1 (modes 1 and 3): the slave changes MISO on the leading SCK edge,
// the master samples on the trailing edge.
static void tx_start(int mode)
{
	tx_stop();
	bool lead_high = (mode == 1);   // mode 1: idle low, leading edge rising. mode 3: idle high, falling.
	tx_instr[0] = pio_encode_wait_gpio(!lead_high, SCK_PIN);   // idle level
	tx_instr[1] = pio_encode_wait_gpio(lead_high, SCK_PIN);    // leading edge
	tx_instr[2] = pio_encode_out(pio_pins, 1);                 // next bit on MISO (autopull, may stall)
	tx_offset = pio_add_program(SNIFF_PIO, &tx_prog);

	pio_sm_config c = pio_get_default_sm_config();
	sm_config_set_wrap(&c, tx_offset, tx_offset + tx_prog.length - 1);
	sm_config_set_out_pins(&c, MISO_PIN, 1);
	sm_config_set_out_shift(&c, false, true, 8);   // shift left = MSB first, autopull every 8 bits
	sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
	sm_config_set_clkdiv(&c, 1.0f);
	pio_sm_init(SNIFF_PIO, SM_TX, tx_offset, &c);
	tx_running = true;
	tx_restart();
}

bool spi_sniff_set_reply(int level)
{
	if (level < -1 || level > 1) {
		return false;
	}
	reply_level = level;
	if (level >= 0) {
		dev_mode = false;   // constant reply and device mode are exclusive
		tx_stop();
	}
	if (enabled) {
		if (level < 0 && !dev_mode) {
			reply_stop();
		} else if (level >= 0) {
			reply_start(level);
		}
	}
	return true;
}

bool spi_sniff_set_device(bool on)
{
	if (on && enabled && !(spi_mode == 1 || spi_mode == 3)) {
		return false;   // only CPHA = 1 modes are supported as a device
	}
	dev_mode = on;
	if (on) {
		reply_level = -1;
	}
	if (enabled) {
		if (on) {
			reply_start(0);
			tx_start(spi_mode);
		} else {
			tx_stop();
			reply_stop();
		}
	}
	return true;
}

bool spi_sniff_set_reg(uint8_t reg, uint8_t value)
{
	if (reg >= SPI_DEV_REG_COUNT) {
		return false;
	}
	regs[reg] = value;
	return true;
}

void spi_sniff_reset_regs(void)
{
	for (int i = 0; i < SPI_DEV_REG_COUNT; i++) {
		regs[i] = 0;
	}
}

void spi_sniff_dump_regs(void)
{
	char line[8 + 3 * SPI_DEV_REG_COUNT + 2];
	int n = snprintf(line, sizeof(line), "REGS");
	for (int i = 0; i < SPI_DEV_REG_COUNT; i++) {
		n += snprintf(line + n, sizeof(line) - n, " %02X", regs[i]);
	}
	n += snprintf(line + n, sizeof(line) - n, "\n");
	my_stdio_usb_out_chars(line, n);
}

bool spi_sniff_enable(int mode)
{
	if (mode < 0 || mode > 3) {
		return false;
	}
	if (dev_mode && !(mode == 1 || mode == 3)) {
		return false;   // device mode needs CPHA = 1
	}
	spi_sniff_disable();

	// Pins stay plain inputs: the Pico only listens (unless reply/device mode drives MISO).
	const uint pins[] = { SCK_PIN, MOSI_PIN, MISO_PIN, CS_PIN };
	for (int i = 0; i < 4; i++) {
		gpio_init(pins[i]);
		gpio_set_dir(pins[i], GPIO_IN);
	}

	// Modes 0 and 3 capture data on the rising SCK edge, modes 1 and 2 on the falling edge.
	bool rising = (mode == 0 || mode == 3);
	prog_instr[0] = pio_encode_wait_gpio(!rising, SCK_PIN);   // wait for the level before the edge
	prog_instr[1] = pio_encode_wait_gpio(rising, SCK_PIN);    // the capture edge
	prog_instr[2] = pio_encode_in(pio_pins, 1);               // sample the data line
	prog_offset = pio_add_program(SNIFF_PIO, &prog);

	sm_setup(SM_MOSI, MOSI_PIN);
	sm_setup(SM_MISO, MISO_PIN);

	spi_sniff_clear();
	byte_idx = 0;
	irq_set_exclusive_handler(SNIFF_IRQ, pio_irq_handler);
	// Highest priority: in device mode the next MISO byte must be queued within a few us
	// of the command byte, USB or GPIO interrupts must not delay it.
	irq_set_priority(SNIFF_IRQ, PICO_HIGHEST_IRQ_PRIORITY);
	pio_set_irq0_source_enabled(SNIFF_PIO, pis_sm0_rx_fifo_not_empty, true);
	irq_set_enabled(SNIFF_IRQ, true);

	enabled = true;
	spi_mode = mode;
	if (dev_mode) {
		reply_start(0);
		tx_start(mode);
	} else if (reply_level >= 0) {
		reply_start(reply_level);
	}
	restart_sms();
	gpio_set_irq_enabled_with_callback(CS_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, cs_callback);
	return true;
}

void spi_sniff_disable(void)
{
	if (!enabled) {
		return;
	}
	gpio_set_irq_enabled(CS_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, false);
	pio_set_sm_mask_enabled(SNIFF_PIO, (1u << SM_MOSI) | (1u << SM_MISO), false);
	pio_set_irq0_source_enabled(SNIFF_PIO, pis_sm0_rx_fifo_not_empty, false);
	irq_set_enabled(SNIFF_IRQ, false);
	irq_remove_handler(SNIFF_IRQ, pio_irq_handler);
	pio_remove_program(SNIFF_PIO, &prog, prog_offset);
	tx_stop();
	reply_stop();   // never leave MISO driven when the analyzer is off
	enabled = false;
}

void spi_sniff_clear(void)
{
	log_tail = log_head;
	dropped = 0;
}

void spi_sniff_status(char *out, int out_len)
{
	if (enabled) {
		// R- passive, R0/RF constant reply, RD register device (keep under 20 chars)
		char r = dev_mode ? 'D' : (reply_level < 0 ? '-' : (reply_level ? 'F' : '0'));
		snprintf(out, out_len, "SPI M%d R%c E%u D%u\n", spi_mode, r,
		         (unsigned)(log_head - log_tail), (unsigned)dropped);
	} else {
		snprintf(out, out_len, "SPI OFF\n");
	}
}

// Print the data bytes of entries [from, to) for one side (shift 0 = MOSI, 8 = MISO).
static void print_side(uint32_t from, uint32_t to, int shift, char *line, int *n)
{
	for (uint32_t i = from; i != to; i++) {
		uint32_t ev = log_buf[i & (LOG_SIZE - 1)];
		if (!(ev & EV_DATA)) {
			continue;
		}
		*n += snprintf(line + *n, 128 - *n, " %02X", (unsigned)((ev >> shift) & 0xFF));
		if (*n > 100) {
			my_stdio_usb_out_chars(line, *n);
			*n = 0;
		}
	}
}

void spi_sniff_dump(void)
{
	char line[128];
	uint32_t frames = 0;
	uint32_t head = log_head;   // snapshot; later events stay for the next dump

	while (log_tail != head) {
		uint32_t start = log_tail;
		bool has_start = (log_buf[start & (LOG_SIZE - 1)] & EV_START) != 0;
		if (has_start) {
			start++;
		}
		// find the end of this frame: a STOP, the next START, or the snapshot head
		uint32_t end = start;
		while (end != head && !(log_buf[end & (LOG_SIZE - 1)] & (EV_STOP | EV_START))) {
			end++;
		}
		bool has_stop = (end != head) && (log_buf[end & (LOG_SIZE - 1)] & EV_STOP);

		int n = snprintf(line, sizeof(line), "%s MO", has_start ? "S" : "-");
		print_side(start, end, 0, line, &n);
		n += snprintf(line + n, sizeof(line) - n, " MI");
		print_side(start, end, 8, line, &n);
		n += snprintf(line + n, sizeof(line) - n, "%s\n", has_stop ? " P" : " -");
		my_stdio_usb_out_chars(line, n);
		frames++;

		log_tail = has_stop ? end + 1 : end;
	}
	int m = snprintf(line, sizeof(line), "END F %u DROP %u\n", (unsigned)frames, (unsigned)dropped);
	my_stdio_usb_out_chars(line, m);
}
