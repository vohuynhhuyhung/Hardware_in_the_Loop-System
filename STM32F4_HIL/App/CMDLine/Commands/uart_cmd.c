/*
 * uart_cmd.c
 *
 *  Created on: Feb 18, 2026
 *      Author: VyTran
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cmd.h"
#include "uart_driver.h"

static int Cmd_UART_Init(int argc, char *argv[])
{
	if (argc != 2)
	    return (argc < 2) ? CMDLINE_TOO_FEW_ARGS : CMDLINE_TOO_MANY_ARGS;

    UART_Init();
    UART_Clear();
    Console_Write("\r\nUART Initialized\r\n");
    return CMDLINE_OK;
}

static int Cmd_UART_Dump_Buffer(int argc, char *argv[])
{
	if (argc != 2)
	    return (argc < 2) ? CMDLINE_TOO_FEW_ARGS : CMDLINE_TOO_MANY_ARGS;

    uint8_t buf[256];
    for (uint16_t i = 0; i < 256; i++)
    {
        buf[i] = i;
        Console_Write("%02X ", buf[i]);
        if ((i + 1) % 16 == 0)
            Console_Write("\r\n");
    }

    UART_Send(buf, 256);
    Console_Write("\r\nUART TX 256 bytes DONE\r\n");
    return CMDLINE_OK;
}

static int Cmd_UART_Receive(int argc, char *argv[])
{
	if (argc != 2)
	    return (argc < 2) ? CMDLINE_TOO_FEW_ARGS : CMDLINE_TOO_MANY_ARGS;

    uint8_t buf[256];
    uint16_t len;

    Console_Write("UART RX DATA:\r\n");

    while ((len = UART_Read(buf, sizeof(buf))) > 0)
    {
        for (uint32_t i = 0; i < len; i++)
        {
            Console_Write("%02X ", buf[i]);

            if ((i + 1) % 16 == 0)
                Console_Write("\r\n");
        }
    }

    Console_Write("\r\n");
    return CMDLINE_OK;
}

static int Cmd_UART_Send_String(int argc, char *argv[])
{
    if (argc < 3) return CMDLINE_TOO_FEW_ARGS;

    for (int i = 1; i < argc; i++)
    {
        UART_Send_String(argv[i]);
        UART_Send_String(" ");
    }

    UART_Send_String("\r\n");

    Console_Write("\r\nUART string sent\r\n");

    return CMDLINE_OK;
}

void Cmd_UART_Register(void)
{
    CLI_RegisterCommand("uart_init", Cmd_UART_Init, "Initialize UART RX interrupt");
    CLI_RegisterCommand("uart_dump", Cmd_UART_Dump_Buffer, "Dump 256 incremental bytes and tranfer");
    CLI_RegisterCommand("uart_rx", Cmd_UART_Receive, "Dump received UART buffer");
    CLI_RegisterCommand("uart_tx", Cmd_UART_Send_String, "Send string via UART | format: uart_send <text>");
}
