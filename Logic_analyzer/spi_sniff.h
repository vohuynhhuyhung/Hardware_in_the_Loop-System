#ifndef SPI_SNIFF_H
#define SPI_SNIFF_H
/*
 * Passive SPI analyzer for the sigrok Pico logic analyzer.
 *
 * Two PIO state machines (pio1) sample MOSI and MISO on the data-capture edge of SCK; a GPIO
 * interrupt on CS delimits the frames. By default the Pico never drives any SPI line, so it can sit
 * on a bus where another device (e.g. the HIL W25Q flash emulator) is the real slave. With Sr00/SrFF
 * a third state machine makes the Pico the slave itself (MISO driven only while CS is low).
 * NOTE: the DUT J3 silkscreen swaps SCK and MOSI. Wire by the STM32 pin names below.
 *
 * Wiring (default):
 *     DUT SCK  (PC10) -> Pico GP10      DUT MISO (PC11) -> Pico GP12
 *     DUT MOSI (PC12) -> Pico GP11      DUT CS   (PD0)  -> Pico GP13
 *     DUT GND         -> Pico GND
 * On the HIL board JP9: columns silk-screened "10", "11", "12" (bottom row) and "13" (top row).
 *
 * USB CDC text commands (end with \n), handled in sr_device.c process_char():
 *     Se<mode>  start sniffing, SPI mode 0..3 (modes 0/3 sample on the rising SCK edge,
 *               modes 1/2 on the falling edge). Also clears the log. MSB first, 8-bit frames.
 *     Sx        stop
 *     Sc        clear the log
 *     Sq        dump: one line per CS frame "S MO 9F 00 00 00 MI FF EF 40 18 P",
 *               then "END F <frames> DROP <n>"
 *     Ss        status line
 *     Sr00      act as the slave: drive MISO low while CS is low (every byte reads 0x00)
 *     SrFF      same, every byte reads 0xFF
 *     Srx       passive again (MISO never driven). Default.
 *               Use Sr00/SrFF ONLY when no other slave is on the bus (MISO contention otherwise).
 *               With a W25Q driver, 0x00 means "not busy", so write commands do not hang the DUT.
 *
 *   Register device mode (the Pico is a small SPI sensor, SPI modes 1 and 3 = CPHA 1 only):
 *     Sd1       device mode on (set it before or after Se), Sd0 off. Excludes Sr00/SrFF.
 *               Frame: first byte = R/W bit (bit 7, 1 = read) + register address (bits 6..0),
 *               then data. Write: data -> regs[addr], regs[addr+1], ...
 *               Read: MISO returns regs[addr], regs[addr+1], ... from the 2nd byte on
 *               (MISO is 0x00 during the command byte). SPI_DEV_REG_COUNT registers (0x00..0x0F);
 *               unmapped registers read 0xFF and ignore writes. Tested at the DUT's ~330 kHz SCK;
 *               the read data is queued from an interrupt, so keep SCK at a few MHz at most.
 *     Sw<rr><vv> set register, Sg dump registers ("REGS .."), Sz reset registers to 0x00.
 *     Registers keep their values across Se/Sx, so a test can preload them.
 *
 *   EEPROM mode (simplified 25LC020A: 256 bytes, 16-byte pages, SPI mode 1 or 3 here):
 *     Sd2       EEPROM device on (Sd0 off). Instructions:
 *                 06 WREN    set the write enable latch (takes effect when CS goes high)
 *                 04 WRDI    clear it
 *                 05 RDSR    status: bit 0 WIP (write in progress), bit 1 WEL. Repeated while CS low.
 *                 03 <a>     READ from address a, auto-increment, wraps 0xFF -> 0x00
 *                 02 <a> <d..> WRITE: needs WEL, else ignored. The address wraps inside the 16-byte
 *                            page (0x1F -> 0x10). On CS high: WIP = 1 for 5 ms, WEL cleared.
 *               While WIP = 1 only RDSR is answered; READ / WRITE / WREN are ignored.
 *               Not emulated: block protection bits (WRSR), HOLD / WP pins.
 *     Sm<aa><vv> set EEPROM byte, Sn dump the EEPROM ("MEM .." 256 bytes), Sy erase (all 0xFF).
 */
#include <stdint.h>
#include <stdbool.h>

#define SPI_DEV_REG_COUNT 16
#define SPI_EE_SIZE       256

#define SPI_DEV_REGS   1   // spi_sniff_set_device() kinds (0 = off)
#define SPI_DEV_EEPROM 2

bool spi_sniff_enable(int mode);
/* -1 passive, 0 drive MISO low, 1 drive MISO high (only while CS is low). Kept across enable/disable. */
bool spi_sniff_set_reply(int level);
/* Device mode: 0 off, SPI_DEV_REGS, SPI_DEV_EEPROM. Kept across enable/disable.
 * False if the current SPI mode has CPHA = 0. */
bool spi_sniff_set_device(int kind);
int  spi_sniff_get_kind(void);
bool spi_sniff_ee_set(uint8_t addr, uint8_t value);
void spi_sniff_ee_erase(void);
void spi_sniff_ee_dump(void);
bool spi_sniff_set_reg(uint8_t reg, uint8_t value);
void spi_sniff_reset_regs(void);
void spi_sniff_dump_regs(void);
void spi_sniff_disable(void);
void spi_sniff_clear(void);
void spi_sniff_dump(void);
/* Short status text (fits the 20-char response buffer), e.g. "SPI M1 RD E12 D0". */
void spi_sniff_status(char *out, int out_len);

#endif /* SPI_SNIFF_H */
