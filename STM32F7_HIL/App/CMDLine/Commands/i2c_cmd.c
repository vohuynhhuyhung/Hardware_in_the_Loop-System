/*
 * i2c_cmd.c
 *
 *  Created on: Sep 30, 2025
 *      Author: VyTran
 */

#include <stdio.h>
#include <stdlib.h>
#include "cmd.h"
#include "i2c_device.h"
#include "i2c_eeprom.h"
#include "i2c_rtc.h"

extern I2C_HandleTypeDef *hi2c;

static int cmd_i2c_dev_active(int argc, char *argv[])
{
    if (argc < 2) {
        Console_Write("Usage: i2c_active <addr>\r\n");
        return CMDLINE_BAD_CMD;
    }

    uint8_t addr = (uint8_t)strtol(argv[1], NULL, 0);
    if (addr == 0 || addr >= 0x7F) {
        Console_Write("Error: Address must be in range [0x01 - 0x7E]\r\n");
        return CMDLINE_OK;
    }

    int rc = i2c_set_slave_addr(addr);
    if (rc != 0) {
        Console_Write("Error: Dev 0x%02X was not initialized\r\n", addr);
        return CMDLINE_OK;
    }

    Console_Write("Dev 0x%02X is now active on I2C bus\r\n", addr);
    return CMDLINE_OK;
}

static int cmd_i2c_rtc_init(int argc, char *argv[])
{
    if (argc < 2) {
        Console_Write("Usage: rtc_init <addr>\r\n");
        return CMDLINE_BAD_CMD;
    }

    uint8_t addr = (uint8_t)strtol(argv[1], NULL, 0);
    if (addr == 0 || addr >= 0x7F) {
        Console_Write("Error: Address must be [0x01 - 0x7E]\r\n");
        return CMDLINE_OK;
    }

    t_rtc *rtc = rtc_init(addr);
    if (!rtc) {
        Console_Write("RTC init failed\r\n");
        return CMDLINE_OK;
    }

    i2c_emu_add_device(I2C_DEV_RTC, addr);

    Console_Write("RTC init OK: addr=0x%02X\r\n", addr);
    return CMDLINE_OK;
}

static int cmd_i2c_rtc_deinit(int argc, char *argv[])
{
    if (argc < 2) {
        Console_Write("Usage: rtc_deinit <addr>\r\n");
        return CMDLINE_BAD_CMD;
    }

    uint8_t addr = (uint8_t)strtol(argv[1], NULL, 0);
    if (addr == 0 || addr >= 0x7F) {
        Console_Write("Error: Address must be [0x01 - 0x7E]\r\n");
        return CMDLINE_OK;
    }

    t_rtc *rtc = find_rtc(addr);
    rtc_deinit(rtc);
    i2c_emu_remove_device(addr);

    Console_Write("RTC 0x%02X deinit OK\r\n", addr);
    return CMDLINE_OK;
}

static int cmd_i2c_rtc_set_time(int argc, char *argv[])
{
    if (argc < 5) {
        Console_Write("Usage: rtc_set_time <addr> <hh> <mm> <ss>\r\n");
        return CMDLINE_BAD_CMD;
    }

    uint8_t addr = (uint8_t)strtol(argv[1], NULL, 0);

    t_rtc *rtc = find_rtc(addr);
    if (!rtc) {
        Console_Write("RTC 0x%02X not init\r\n", addr);
        return CMDLINE_OK;
    }

    rtc_time_t t;
    t.hour = atoi(argv[2]);
    t.min  = atoi(argv[3]);
    t.sec  = atoi(argv[4]);

    rtc_set_time(rtc, &t);
    Console_Write("Time set: %02d:%02d:%02d\r\n", t.hour, t.min, t.sec);

    return CMDLINE_OK;
}

static int cmd_i2c_rtc_set_date(int argc, char *argv[])
{
    if (argc < 5) {
        Console_Write("Usage: rtc_set_date <addr> <dd> <mm> <yy>\r\n");
        return CMDLINE_BAD_CMD;
    }

    uint8_t addr = (uint8_t)strtol(argv[1], NULL, 0);

    t_rtc *rtc = find_rtc(addr);
    if (!rtc) {
        Console_Write("RTC 0x%02X not init\r\n", addr);
        return CMDLINE_OK;
    }

    rtc_date_t d;
    d.day   = atoi(argv[2]);
    d.month = atoi(argv[3]);
    d.year  = atoi(argv[4]);

    rtc_set_date(rtc, &d);
    Console_Write("Date set: %02d/%02d/%02d\r\n", d.day, d.month, d.year);

    return CMDLINE_OK;
}

static int cmd_i2c_rtc_get_time(int argc, char *argv[])
{
    if (argc < 2) {
        Console_Write("Usage: rtc_get_time <addr>\r\n");
        return CMDLINE_BAD_CMD;
    }

    uint8_t addr = (uint8_t)strtol(argv[1], NULL, 0);

    t_rtc *rtc = find_rtc(addr);
    if (!rtc) {
        Console_Write("RTC 0x%02X not init\r\n", addr);
        return CMDLINE_OK;
    }

    rtc_time_t t;
    rtc_get_time(rtc, &t);

    Console_Write("Time: %02d:%02d:%02d\r\n", t.hour, t.min, t.sec);
    return CMDLINE_OK;
}

static int cmd_i2c_rtc_get_date(int argc, char *argv[])
{
    if (argc < 2) {
        Console_Write("Usage: rtc_get_date <addr>\r\n");
        return CMDLINE_BAD_CMD;
    }

    uint8_t addr = (uint8_t)strtol(argv[1], NULL, 0);

    t_rtc *rtc = find_rtc(addr);
    if (!rtc) {
        Console_Write("RTC 0x%02X not init\r\n", addr);
        return CMDLINE_OK;
    }

    rtc_date_t d;
    rtc_get_date(rtc, &d);

    Console_Write("Date: %02d/%02d/%02d\r\n", d.day, d.month, d.year);
    return CMDLINE_OK;
}

static int cmd_i2c_eeprom_init(int argc, char *argv[])
{
   if (argc < 4) {
		Console_Write("Usage: eeprom_init <addr> <size> <page_size>\r\n");
		return CMDLINE_BAD_CMD;
	}

	uint8_t addr = (uint8_t)strtol(argv[1], NULL, 0);
	if (addr == 0 || addr >= 0x7F) {
		Console_Write("Error: Address must be [0x01 - 0x7E]\r\n");
		return CMDLINE_OK;
	}

	uint16_t size = (uint16_t)strtol(argv[2], NULL, 0);
	uint16_t psize = (uint16_t)strtol(argv[3], NULL, 0);

	t_eeprom *e = eeprom_init(hi2c, addr, size, psize);
    if (!e) {
        Console_Write("EEPROM init failed\r\n");
        return CMDLINE_OK;
    }

    i2c_emu_add_device(I2C_DEV_EEPROM, addr);
    Console_Write("EEPROM init OK: addr=0x%02X, size=%u, page=%u\r\n", addr, size, psize);

	return CMDLINE_OK;
}

static int cmd_i2c_eeprom_deinit(int argc, char *argv[]) {
   if (argc < 2) {
		Console_Write("Usage: eeprom_deinit <addr>\r\n");
		return CMDLINE_BAD_CMD;
	}

	uint8_t addr = (uint8_t)strtol(argv[1], NULL, 0);
	if (addr == 0 || addr >= 0x7F) {
		Console_Write("Error: Address must be [0x01 - 0x7E]\r\n");
		return CMDLINE_OK;
	}

	t_eeprom *e = find_eeprom(addr);
    if (!e) {
        Console_Write("EEPROM 0x%02X not init\r\n", addr);
        return CMDLINE_OK;
    }

    eeprom_deinit(e);
    i2c_emu_remove_device(addr);

    Console_Write("EEPROM 0x%02X deinit OK\r\n", addr);

	return CMDLINE_OK;
}


static int cmd_i2c_eeprom_find(int argc, char *argv[]) {
   if (argc < 2) {
		Console_Write("Usage: eeprom_find <addr>\r\n");
		return CMDLINE_BAD_CMD;
	}

	uint8_t addr = (uint8_t)strtol(argv[1], NULL, 0);
	if (addr == 0 || addr >= 0x7F) {
		Console_Write("Error: Address must be [0x01 - 0x7E]\r\n");
		return CMDLINE_OK;
	}

	t_eeprom *e = find_eeprom(addr);
    if (!e) {
        Console_Write("EEPROM 0x%02X not init\r\n", addr);
        return CMDLINE_OK;
    }

    Console_Write("EEPROM 0x%02X:\r\n", e->eeprom_addr);
    Console_Write("  Size      = %u\r\n", e->eeprom_size);
    Console_Write("  Page size = %u\r\n", e->page_size);

	return CMDLINE_OK;
}

void Cmd_I2C_Register(void)
{
    CLI_RegisterCommand("i2c_dev_active",  cmd_i2c_dev_active,      "Active device on bus");

    CLI_RegisterCommand("eeprom_init",     cmd_i2c_eeprom_init,     "Init I2C EEPROM emulator");
    CLI_RegisterCommand("eeprom_deinit",   cmd_i2c_eeprom_deinit,   "Deinit I2C EEPROM");
    CLI_RegisterCommand("eeprom_find",     cmd_i2c_eeprom_find,     "Find EEPROM info");

    CLI_RegisterCommand("rtc_init",      cmd_i2c_rtc_init,      "Init I2C RTC emulator");
    CLI_RegisterCommand("rtc_deinit",    cmd_i2c_rtc_deinit,    "Deinit I2C RTC");

    CLI_RegisterCommand("rtc_set_time",  cmd_i2c_rtc_set_time,  "Set RTC time");
    CLI_RegisterCommand("rtc_set_date",  cmd_i2c_rtc_set_date,  "Set RTC date");

    CLI_RegisterCommand("rtc_get_time",  cmd_i2c_rtc_get_time,  "Get RTC time");
    CLI_RegisterCommand("rtc_get_date",  cmd_i2c_rtc_get_date,  "Get RTC date");
}
