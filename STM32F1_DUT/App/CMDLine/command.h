/*
 * command.h
 *
 *  Created on: Nov 21, 2024
 *      Author: Samy Ve
 */

#ifndef CMDLINE_COMMAND_H_
#define CMDLINE_COMMAND_H_

#include "cmdline.h"
#include "uart.h"
#include "main.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define	COMMAND_MAX_LENGTH	64
#define MAX_HISTORY 8
#define MAX_CMD_LENGTH COMMAND_MAX_LENGTH

extern USART_TypeDef *UART_CMDLINE;

void CommandLine_Task(void *pvParameters);

void CommandLine_Init(USART_TypeDef *handle_uart);
void CommandLine_CreateTask(void);
void Command_SendSplash(void);

/* Command support */
int Cmd_help(int argc, char *argv[]);
int Cmd_Clear_CLI(int argc, char *argv[]);
int Cmd_ReadID_W25Q(int argc, char *argv[]);
int Cmd_Write_W25Q(int argc, char *argv[]);
int Cmd_Read_W25Q(int argc, char *argv[]);
int Cmd_EraseChip_W25Q(int argc, char *argv[]);
int Cmd_EEPROM_Init(int argc, char *argv[]);
int Cmd_EEPROM_Write(int argc, char *argv[]);
int Cmd_EEPROM_Read(int argc, char *argv[]);
int Cmd_EEPROM_Fill(int argc, char *argv[]);

#endif /* CMDLINE_COMMAND_H_ */
