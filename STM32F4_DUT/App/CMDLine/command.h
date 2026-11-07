/*
 * command.h
 *
 *  Created on: Nov 21, 2024
 *      Author: Samy Ve
 */

#ifndef CMDLINE_COMMAND_H_
#define CMDLINE_COMMAND_H_

#include "cmdline.h"
#include "main.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define	COMMAND_MAX_LENGTH	64
#define MAX_HISTORY 8
#define MAX_CMD_LENGTH COMMAND_MAX_LENGTH

void CommandLine_Task(void *pvParameters);

void CommandLine_Init(void);
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
int Cmd_I2C_Scan(int argc, char *argv[]);
int Cmd_set_freq(int argc, char *argv[]);
int Cmd_get_freq(int argc, char *argv[]);
int Cmd_Sine_Wave(int argc, char *argv[]);
int Cmd_Triangle_Wave(int argc, char *argv[]);
int Cmd_UART_Init(int argc, char *argv[]);
int Cmd_UART_Dump_Buffer(int argc, char *argv[]);
int Cmd_UART_Receive(int argc, char *argv[]);
int Cmd_UART_Send_String(int argc, char *argv[]);
int Cmd_ADC_Read(int argc, char *argv[]);
int Cmd_PWM_Freq(int argc, char *argv[]);
int Cmd_PWM_Duty_Cycle(int argc, char *argv[]);
int Cmd_PWM_Start(int argc, char *argv[]);
int Cmd_PWM_Stop(int argc, char *argv[]);
int Cmd_Set_Led(int argc, char *argv[]);
int Cmd_Read_Sine_Frequency(int argc, char *argv[]);
int Cmd_RTC_Init(int argc, char *argv[]);
int Cmd_RTC_Set_Time(int argc, char *argv[]);
int Cmd_RTC_Get_Time(int argc, char *argv[]);
int Cmd_RTC_Set_Date(int argc, char *argv[]);
int Cmd_RTC_Get_Date(int argc, char *argv[]);

#endif /* CMDLINE_COMMAND_H_ */
