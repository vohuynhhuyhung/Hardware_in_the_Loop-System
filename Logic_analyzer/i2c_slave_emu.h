#ifndef I2C_SLAVE_EMU_H
#define I2C_SLAVE_EMU_H
/*
 * I2C device emulation for the sigrok Pico logic analyzer.
 *
 * The Pico behaves like a small register-based I2C device (e.g. a sensor) at a chosen 7-bit
 * address. The RP2350 I2C block ACKs the address and every data byte. Protocol, as on most
 * sensors / EEPROMs with an 8-bit register pointer:
 *     write  [reg] [d0] [d1] ...           -> regs[reg] = d0, regs[reg+1] = d1, ...
 *     write  [reg], repeated START, read n -> regs[reg], regs[reg+1], ...
 *     read n (no register byte first)      -> continues from the current register pointer
 * There are I2C_EMU_REG_COUNT registers (0x00..0x12). Unmapped registers read 0xFF and ignore writes.
 * Every START / byte / STOP is also logged and can be dumped as text.
 *
 * Wiring (default, i2c0 - GP0/GP1 are free in BASE_MODE because UART_EN is 0):
 *     DUT SDA  ->  Pico GP0   (pin 1)
 *     DUT SCL  ->  Pico GP1   (pin 2)
 *     DUT GND  ->  Pico GND
 * The bus needs pull-ups (the DUT board already has 4.7k to 3.3 V). The internal pull-ups
 * of the Pico are intentionally NOT enabled.
 *
 * USB CDC text commands (end with \n), handled in sr_device.c process_char():
 *     Ie<hh>      enable the device at 7-bit address <hh> (hex, 08..77), e.g. "Ie68". Clears the log.
 *     Ix          disable
 *     Ic          clear the event log
 *     Iq          dump the event log as text, ends with a line "END DROP <n>"
 *     Is          status line
 *     Iw<rr><vv>  set register rr to vv (hex), e.g. "Iw0F68"
 *     Ig          dump all registers: "REGS 68 00 ..."
 *     Iz          reset all registers to 0x00
 *     It1 / It0   DS3231 RTC mode on / off (see below)
 * Registers keep their values across Ie/Ix, so a test can preload them before enabling.
 *
 * DS3231 RTC mode (simplified): use with address 0x68 ("Ie68", "It1", "Iz").
 *     0x00 seconds  0x01 minutes  0x02 hours (24 h)  0x03 day 1..7  0x04 date  0x05 month (bit 7 = century)
 *     0x06 year 00..99   (all BCD) - these advance every second, with month lengths and leap years.
 *     0x07..0x12 (alarms, control 0x0E, status 0x0F, aging 0x10, temperature 0x11..0x12) are plain
 *     storage: no alarms, no 12 h mode, no temperature conversion.
 *     "Iz" in RTC mode loads the DS3231 power-on state: 2000-01-01 00:00:00, day 1.
 */
#include <stdint.h>
#include <stdbool.h>

#define I2C_EMU_REG_COUNT 19   // 0x00..0x12 = the DS3231 register map

bool i2c_emu_enable(uint8_t addr7);
void i2c_emu_disable(void);
void i2c_emu_clear(void);
bool i2c_emu_set_reg(uint8_t reg, uint8_t value);
int  i2c_emu_get_reg(uint8_t reg);
void i2c_emu_reset_regs(void);
/* DS3231 RTC mode: the time registers 0x00..0x06 run. */
void i2c_emu_rtc_enable(bool on);
bool i2c_emu_rtc_is_on(void);
/* Dump and consume the log to the USB CDC. */
void i2c_emu_dump(void);
/* Print the register map to the USB CDC. */
void i2c_emu_dump_regs(void);
/* Short status text (fits the 20-char response buffer), e.g. "I2C ON 42 E12 D0" = events, dropped. */
void i2c_emu_status(char *out, int out_len);

#endif /* I2C_SLAVE_EMU_H */
