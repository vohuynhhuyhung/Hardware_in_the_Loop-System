/*
* cmd.h
*
* Created on: Sep 28, 2025
* Author: VyTran
*
*/

#ifndef CMDLINE_COMMAND_H_
#define CMDLINE_COMMAND_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "cmdline.h"

#define COMMAND_MAX_LENGTH 64
#define MAX_HISTORY 8
#define MAX_CMDS 32

#define NAME_SHELL "HIL:~ "
#define KEY_ENTER '\r'
#define KEY_BACKSPACE '\x7f'

// Abstract console interface
void Console_Init(void);
void Console_Write(const char *fmt, ...);
bool Console_Available(void);
char Console_Read(void);

// Command task
void CLI_Task(void *pvParameters);
void CLI_Init(void);

// Transfer helper APIs
// Để chạy được USB CDC phải gọi api này cuối hàm CDC_Receive_FS trong file usb_cdc_if.c
uint8_t CDC_ReceiveCallback (uint8_t* Buf, uint32_t *Len);

// Register commands APIs
void CLI_RegisterCommand(const char *cmd, int (*handler)(int, char **), const char *help);
void Cmd_SPI_Register(void);
void Cmd_I2C_Register(void);

#endif /* CMDLINE_COMMAND_H_ */
