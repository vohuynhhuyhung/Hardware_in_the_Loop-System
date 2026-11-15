#include "uart_rx_emu.h"

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/uart.h"

// Defined in pico_sdk_sigrok.c: fast write to the USB CDC.
extern void my_stdio_usb_out_chars(const char *buf, int length);

#define EMU_UART     uart1
#define EMU_IRQ      UART1_IRQ
#define EMU_RX_PIN   21   // uart1 RX

#define LOG_SIZE 2048    // bytes, power of two (4 KB of RAM). Keep small: the LA mallocs 476 KB at boot

// One entry = data byte (bits 0..7) + UART error flags from UARTDR (bits 8..11: FE, PE, BE, OE)
static volatile uint16_t log_buf[LOG_SIZE];
static volatile uint32_t log_head;   // written by the IRQ
static volatile uint32_t log_tail;   // written by the main loop
static volatile uint32_t dropped;
static bool enabled;
static bool irq_registered;

static void uart_emu_irq(void)
{
	uart_hw_t *hw = uart_get_hw(EMU_UART);
	while (!(hw->fr & UART_UARTFR_RXFE_BITS)) {
		uint16_t v = (uint16_t)(hw->dr & 0xFFF);   // reading DR pops the FIFO, keeps error bits
		uint32_t h = log_head;
		if ((h - log_tail) >= LOG_SIZE) {
			dropped++;
			continue;
		}
		log_buf[h & (LOG_SIZE - 1)] = v;
		log_head = h + 1;
	}
	hw->rsr = 0;   // clear the receive status / error register
}

bool uart_emu_enable(uint32_t baud)
{
	if (baud < 300 || baud > 3000000) {
		return false;
	}
	uart_emu_disable();

	// Pull the line up and route it BEFORE the receiver runs: otherwise the receiver sees the
	// floating/low pad for a moment and logs a spurious 00 byte with framing + break errors.
	gpio_pull_up(EMU_RX_PIN);   // idle-high line stays defined if the DUT is unpowered
	gpio_set_function(EMU_RX_PIN, GPIO_FUNC_UART);
	sleep_us(200);

	uart_init(EMU_UART, baud);
	uart_set_format(EMU_UART, 8, 1, UART_PARITY_NONE);
	uart_set_hw_flow(EMU_UART, false, false);
	uart_set_fifo_enabled(EMU_UART, true);

	// Discard anything captured while the line settled, then start with an empty log.
	sleep_us(200);
	uart_hw_t *hw = uart_get_hw(EMU_UART);
	while (!(hw->fr & UART_UARTFR_RXFE_BITS)) {
		(void)hw->dr;
	}
	hw->rsr = 0;
	uart_emu_clear();

	if (!irq_registered) {
		irq_set_exclusive_handler(EMU_IRQ, uart_emu_irq);
		irq_registered = true;
	}
	irq_set_enabled(EMU_IRQ, true);
	uart_set_irq_enables(EMU_UART, true, false);   // RX + RX timeout interrupts, no TX

	enabled = true;
	return true;
}

void uart_emu_disable(void)
{
	if (!enabled) {
		return;
	}
	uart_set_irq_enables(EMU_UART, false, false);
	irq_set_enabled(EMU_IRQ, false);
	uart_deinit(EMU_UART);
	gpio_disable_pulls(EMU_RX_PIN);
	gpio_init(EMU_RX_PIN);   // back to a plain input for the logic analyzer
	enabled = false;
}

void uart_emu_clear(void)
{
	log_tail = log_head;
	dropped = 0;
}

void uart_emu_status(char *out, int out_len)
{
	if (enabled) {
		snprintf(out, out_len, "UART ON N%u D%u\n",
		         (unsigned)(log_head - log_tail), (unsigned)dropped);
	} else {
		snprintf(out, out_len, "UART OFF\n");
	}
}

void uart_emu_dump(void)
{
	char line[128];
	int n = 0, per_line = 0;
	uint32_t count = 0, fe = 0, pe = 0, be = 0, oe = 0;

	while (log_tail != log_head) {
		uint16_t v = log_buf[log_tail & (LOG_SIZE - 1)];
		log_tail = log_tail + 1;
		count++;
		if (v & UART_UARTDR_FE_BITS) fe++;
		if (v & UART_UARTDR_PE_BITS) pe++;
		if (v & UART_UARTDR_BE_BITS) be++;
		if (v & UART_UARTDR_OE_BITS) oe++;

		if (per_line == 0) {
			n += snprintf(line + n, sizeof(line) - n, "RX");
		}
		n += snprintf(line + n, sizeof(line) - n, " %02X", (unsigned)(v & 0xFF));
		if (++per_line == 32) {
			n += snprintf(line + n, sizeof(line) - n, "\n");
			my_stdio_usb_out_chars(line, n);
			n = 0;
			per_line = 0;
		}
	}
	if (per_line) {
		n += snprintf(line + n, sizeof(line) - n, "\n");
		my_stdio_usb_out_chars(line, n);
	}
	int m = snprintf(line, sizeof(line), "END N %u FE %u PE %u BE %u OE %u DROP %u\n",
	                 (unsigned)count, (unsigned)fe, (unsigned)pe, (unsigned)be, (unsigned)oe,
	                 (unsigned)dropped);
	my_stdio_usb_out_chars(line, m);
}
