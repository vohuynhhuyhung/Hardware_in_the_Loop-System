#ifndef UART_RX_EMU_H
#define UART_RX_EMU_H
/*
 * UART receiver for the sigrok Pico logic analyzer.
 *
 * The Pico receives what the DUT transmits, stores every byte together with the line
 * errors reported by the UART (framing, parity, break, overrun) and dumps it as text.
 * A wrong baud rate shows up as framing errors / garbage bytes.
 *
 * Wiring (default, uart1 RX on GP21 = logic analyzer channel D19, not used by the LA tests):
 *     DUT TX   ->  Pico GP21  (Pico pin 27; on the HIL board: JP9 pin 27 = top-row column
 *                               silk-screened "21". NOT JP9 pin 30, which is GP27/ADC1)
 *     DUT GND  ->  Pico GND
 * Only RX is used: the Pico never drives the line. DUT TX may stay connected to the HIL too.
 *
 * USB CDC text commands (end with \n), handled in sr_device.c process_char():
 *     Ue<baud>  enable 8N1 receiver, e.g. "Ue115200" (also clears the log)
 *     Ux        disable
 *     Uc        clear the log
 *     Uq        dump: lines "RX xx xx ...", then "END N <n> FE <n> PE <n> BE <n> OE <n> DROP <n>"
 *     Us        status line
 */
#include <stdint.h>
#include <stdbool.h>

bool uart_emu_enable(uint32_t baud);
void uart_emu_disable(void);
void uart_emu_clear(void);
void uart_emu_dump(void);
/* Short status text (fits the 20-char response buffer), e.g. "UART ON N12 D0". */
void uart_emu_status(char *out, int out_len);

#endif /* UART_RX_EMU_H */
