/*
* command.c
*
*  Created on: Nov 21, 2024
*      Author: Samy Ve
*/

#include "command.h"
#include "cmdline.h"
#include "uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_rcc.h"
#include <stdlib.h>
#include <stdio.h>
#include "w25q_driver.h"


// Add history
// Add some utils to CommandLine
#define NAME_SHELL "DUT:~ "
#define KEY_ENTER '\r'       /* [enter] key */
#define KEY_BACKSPACE '\x7f' /* [backspace] key */

USART_TypeDef *UART_CMDLINE;

typedef struct {
	char commandBuffer[COMMAND_MAX_LENGTH];
	uint16_t commandBufferIndex;
	char commandHistory[MAX_HISTORY][COMMAND_MAX_LENGTH];
	uint16_t historyCount;
	uint16_t historyIndex;
} CMDLine_Context;

CMDLine_Context pContext = { 0 };

/* Private typedef -----------------------------------------------------------*/

/* Private function ----------------------------------------------------------*/
static void CommandLine_Task_Update(void);
void process_command(char rxData, CMDLine_Context *context);

/* Private variable -----------------------------------------------------------*/

const char *ErrorCode[6] = { "OK\r\n", "BAD_CMD\r\n",
		"TOO_MANY_ARGS\r\n", "TOO_FEW_ARGS\r\n",
		"INVALID_ARG\r\n", "CMD_OK_BUT_PENDING...\r\n" };

//extern SPI_HandleTypeDef hspi2;

static char s_commandBuffer[COMMAND_MAX_LENGTH];
static uint8_t s_commandBufferIndex = 0;

tCmdLineEntry g_psCmdTable[] =
		{
		/* Command support */
		{ "help", Cmd_help, "Display list of available commands | format: help" },
		{ "cls", Cmd_Clear_CLI, "Clear Console | format: cls" },

		{ "w25q_ID", Cmd_ReadID_W25Q, "Read ID W25Q | format: w25q_readID" },
		{ "w25q_write", Cmd_Write_W25Q, "Write data to W25Q from addr 0 to <addr> | format: w25q_write <data>" },
		{ "w25q_read", Cmd_Read_W25Q, "Read data from W25Q at addr 0 to <size> | format: w25q_read <size>" },
		{ "w25q_erasechip", Cmd_EraseChip_W25Q, "Erase entire W25Q chip | format: w25q_erasechip" },

		{ "eeprom_init",  Cmd_EEPROM_Init,  "eeprom_init <addr7bit> <size> <page>" },
		{ "eeprom_write", Cmd_EEPROM_Write, "eeprom_write <addr> <len>" },
		{ "eeprom_read",  Cmd_EEPROM_Read,  "eeprom_read <addr> <len>" },
		{ "eeprom_fill",  Cmd_EEPROM_Fill,  "eeprom_fill <addr> <len>" },

		{ 0, 0, 0 } };

void CommandLine_Task(void *pvParameters)
{
    while (1)
    {
        CommandLine_Task_Update();   // gọi hàm xử lý CLI
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void CommandLine_Init(USART_TypeDef *handle_uart) {
	UART_CMDLINE = handle_uart;
	memset((void*) s_commandBuffer, 0, sizeof(s_commandBuffer));
	s_commandBufferIndex = 0;
	UART_SendStringRing(UART_CMDLINE, "\n\n\rDUT FIRMWARE \r\n");
	UART_Flush_RingRx(UART_CMDLINE);

	char buffer[30];
	snprintf(buffer, sizeof(buffer), "\r%s$ ", NAME_SHELL);
	UART_SendStringRing(UART_CMDLINE, buffer);
}

static void CommandLine_Task_Update(void) {
	char rxData;
	if (IsDataAvailable(UART_CMDLINE)) {
		rxData = UART_ReadRing(UART_CMDLINE);
		if (rxData == 27) {
			UART_SendStringRing(UART_CMDLINE, "\033[2J");
		} else {
			UART_WriteRing(UART_CMDLINE, rxData);
		}
		process_command(rxData, &pContext);
	}
}

void process_command(char rxData, CMDLine_Context *context) {
	if (rxData == 27) {
		char buffer[60];
		snprintf(buffer, sizeof(buffer), "\r\n%s$ ", NAME_SHELL);
		UART_SendStringRing(UART_CMDLINE, buffer);
		context->commandBufferIndex = 0;
		context->commandBuffer[0] = '\0';
	}

	if (rxData == 0x2D) // '-' key (history up)
	{
		if (context->historyIndex > 0) {
			context->historyIndex--;
		}

		// Load history command
		if (context->historyIndex < context->historyCount) {
			strcpy(context->commandBuffer,
					context->commandHistory[context->historyIndex]);
			context->commandBufferIndex = strlen(context->commandBuffer);
		} else {
			context->commandBuffer[0] = '\0';
			context->commandBufferIndex = 0;
		}

		// Clear current line and display updated command
		UART_SendStringRing(UART_CMDLINE, "\033[2K"); // Clear entire line
		char buffer[60];
		snprintf(buffer, sizeof(buffer), "\r\n%s$ ", NAME_SHELL);
		UART_SendStringRing(UART_CMDLINE, buffer);
		UART_SendStringRing(UART_CMDLINE, context->commandBuffer); // Display updated command
		return;
	} else if (rxData == 0x3D) // '=' key (history down)
	{
		if (context->historyIndex < context->historyCount) {
			context->historyIndex++;
		}

		// Load history command
		if (context->historyIndex < context->historyCount) {
			strcpy(context->commandBuffer,
					context->commandHistory[context->historyIndex]);
			context->commandBufferIndex = strlen(context->commandBuffer);
		} else {
			context->commandBuffer[0] = '\0';
			context->commandBufferIndex = 0;
		}

		// Clear current line and display updated command
		UART_SendStringRing(UART_CMDLINE, "\033[2K"); // Clear entire line
		char buffer[60];
		snprintf(buffer, sizeof(buffer), "\r\n%s$ ", NAME_SHELL);
		UART_SendStringRing(UART_CMDLINE, buffer);
		UART_SendStringRing(UART_CMDLINE, context->commandBuffer); // Display updated command
		return;
	}

	// Handle individual key presses
	if (((rxData >= 32 && rxData <= 126) || rxData == KEY_ENTER
			|| rxData == KEY_BACKSPACE) && rxData != 0x2D && rxData != 0x3D
			&& rxData != 0x5C) {
		if (rxData == KEY_ENTER) {
			if (context->commandBufferIndex > 0) {
				context->commandBuffer[context->commandBufferIndex] = '\0';
				// Save to history
				if (context->historyCount == 0
						|| strcmp(
								context->commandHistory[context->historyCount
										- 1], context->commandBuffer) != 0) {
					if (context->historyCount < MAX_HISTORY) {
						strcpy(context->commandHistory[context->historyCount],
								context->commandBuffer);
						context->historyCount++;
					} else {
						for (int i = 0; i < MAX_HISTORY - 1; i++) {
							strcpy(context->commandHistory[i],
									context->commandHistory[i + 1]);
						}
						strcpy(context->commandHistory[MAX_HISTORY - 1],
								context->commandBuffer);
					}
				}
				context->historyIndex = context->historyCount;

				// Process command
				int8_t ret_val = CmdLineProcess(context->commandBuffer);
				if (ret_val == CMDLINE_NONE_RETURN) {
				} else {
					char buffer[60];
					if (ret_val != CMDLINE_OK)
					{
						snprintf(buffer, sizeof(buffer), "\r\n");
						UART_SendStringRing(UART_CMDLINE, buffer);
						UART_SendStringRing(UART_CMDLINE, ErrorCode[ret_val]);
					}
					snprintf(buffer, sizeof(buffer), "\r\n%s$ ", NAME_SHELL);
					UART_SendStringRing(UART_CMDLINE, buffer);
					context->commandBufferIndex = 0;
				}
			} else {
				char buffer[60];
				snprintf(buffer, sizeof(buffer), "\r\n%s$ ", NAME_SHELL);
				UART_SendStringRing(UART_CMDLINE, buffer);
			}
		} else if (rxData == KEY_BACKSPACE) {
			if (context->commandBufferIndex > 0) {
				context->commandBufferIndex--;
				context->commandBuffer[context->commandBufferIndex] = '\0';
			} else {
				UART_SendStringRing(UART_CMDLINE, " ");
			}
		} else {
			if (context->commandBufferIndex < COMMAND_MAX_LENGTH - 1) {
				context->commandBuffer[context->commandBufferIndex++] = rxData;
				context->commandBuffer[context->commandBufferIndex] = '\0';
			} else {
				// Command too long
				UART_SendStringRing(UART_CMDLINE,
						"\r\nError: Command too long.");
				char buffer[60];
				snprintf(buffer, sizeof(buffer), "\r\n%s$ ", NAME_SHELL);
				UART_SendStringRing(UART_CMDLINE, buffer);
				context->commandBufferIndex = 0;
				context->commandBuffer[0] = '\0';
			}
		}
	}
}

/*-----------------------COMMAND FUNCTION LIST---------------------------*/
/* Command support */
int Cmd_help(int argc, char *argv[]) {
	if (argc > 2) return CMDLINE_TOO_MANY_ARGS;
	tCmdLineEntry *pEntry = &g_psCmdTable[0];
	size_t maxCmdLength = 0;
	while (pEntry->pcCmd) {
		size_t cmdLength = strlen(pEntry->pcCmd);
		if (cmdLength > maxCmdLength) {
			maxCmdLength = cmdLength;
		}
		pEntry++;
	}
	pEntry = &g_psCmdTable[0];
	while (pEntry->pcCmd) {
		char buffer[256];
		size_t cmdLength = strlen(pEntry->pcCmd);
		int padding = (int) (maxCmdLength - cmdLength + 2);
		snprintf(buffer, sizeof(buffer), "\r\n%s%*s %s", pEntry->pcCmd,
				padding, "", pEntry->pcHelp);
		UART_SendStringRing(UART_CMDLINE, buffer);
		pEntry++;
	}
	return (CMDLINE_OK);
}

int Cmd_Clear_CLI(int argc, char *argv[]) {
	if (argc > 2) return CMDLINE_TOO_MANY_ARGS;
	UART_SendStringRing(UART_CMDLINE, "\033[2J\033[H");
	return (CMDLINE_OK);
}

int Cmd_ReadID_W25Q(int argc, char *argv[]) {
	if (argc > 2) return CMDLINE_TOO_MANY_ARGS;
	uint8_t id[4];
	char buffer[30];
	W25Q_read_id(&W25Q, id);
	snprintf(buffer, sizeof(buffer), "\r\nW25Q ID: 0x%X%X%X%x\r\n", id[0], id[1], id[2], id[3]);
	UART_SendStringRing(UART_CMDLINE, buffer);
	return (CMDLINE_OK);
}

int Cmd_Write_W25Q(int argc, char *argv[]) {
    if (argc < 3)
        return CMDLINE_TOO_FEW_ARGS;
    if (argc > 3)
        return CMDLINE_TOO_MANY_ARGS;

    char buffer[64];
    uint32_t length = atoi(argv[1]);  // số byte cần ghi

    if (length == 0 || length > 1024) {  // Giới hạn để tránh buffer quá lớn
        UART_SendStringRing(UART_CMDLINE, "\r\nInvalid length\r\n");
        return CMDLINE_OK;
    }

    // Tạo buffer dữ liệu: 0, 1, 2, 3, ...
    static uint8_t test_data[1024]; // đủ lớn
    for (uint32_t i = 0; i < length; i++) {
        test_data[i] = (uint8_t)(i & 0xFF);
    }

    // Ghi vào flash
	W25Q_write(&W25Q, 0x000000, length, test_data);

    snprintf(buffer, sizeof(buffer), "\r\nWrite %lu bytes from addr [0 - %lu] to W25Q OK\r\n",
             (unsigned long)length, (unsigned long)(length - 1));
    UART_SendStringRing(UART_CMDLINE, buffer);

    return CMDLINE_OK;
}

int Cmd_Read_W25Q(int argc, char *argv[]) {
	if (argc < 3)
		return CMDLINE_TOO_FEW_ARGS;
	if (argc > 3)
		return CMDLINE_TOO_MANY_ARGS;

	char buffer[128];
	uint32_t length = atoi(argv[1]);  // số byte cần đọc

	if (length == 0 || length > 1024) {  // Giới hạn để tránh buffer quá lớn
		UART_SendStringRing(UART_CMDLINE, "\r\nInvalid length\r\n");
		return CMDLINE_OK;
	}

	// Tạo buffer để đọc dữ liệu
	static uint8_t read_data[1024]; // đủ lớn

	// Đọc từ flash
	W25Q_read(&W25Q, 0x000000, length, read_data);

	// Hiển thị dữ liệu đọc được
	snprintf(buffer, sizeof(buffer), "\r\nRead %lu bytes from addr [0 - %lu] from W25Q:\r\n",
			 (unsigned long)length, (unsigned long)(length - 1));
	UART_SendStringRing(UART_CMDLINE, buffer);
	for (uint32_t i = 0; i < length; i++) {
		snprintf(buffer, sizeof(buffer), "0x%02X ", read_data[i]);
		UART_SendStringRing(UART_CMDLINE, buffer);
		if ((i + 1) % 16 == 0) {
			UART_SendStringRing(UART_CMDLINE, "\r\n");
		}
	}
	UART_SendStringRing(UART_CMDLINE, "\r\n");

	return CMDLINE_OK;
}

int Cmd_EraseChip_W25Q(int argc, char *argv[]) {
	if (argc > 2)
		return CMDLINE_TOO_MANY_ARGS;

	char buffer[30];
	// Xóa chip
	W25Q_chip_erase(&W25Q);
	snprintf(buffer, sizeof(buffer), "\r\nChip Erase OK\r\n");
	UART_SendStringRing(UART_CMDLINE, buffer);

	return CMDLINE_OK;
}

#include "i2c_eeprom.h"

extern I2C_HandleTypeDef hi2c2;
static I2C_EEPROM_t EEPROM1;

int Cmd_EEPROM_Init(int argc, char *argv[])
{
    if (argc > 5)
        return CMDLINE_INVALID_ARG;

    uint8_t  addr7 = strtol(argv[1], NULL, 0);
    uint16_t size  = atoi(argv[2]);
    uint16_t page  = atoi(argv[3]);

    if (EEPROM_Init(&EEPROM1,
                    &hi2c2,
                    addr7,
                    size,
                    page,
                    I2C_MEMADD_SIZE_16BIT) != HAL_OK)
    {
        UART_SendStringRing(UART_CMDLINE, "\r\nEEPROM init FAIL\r\n");
        return CMDLINE_OK;
    }

    char buf[64];
    snprintf(buf, sizeof(buf),
             "\r\nEEPROM init OK: addr=0x%02X size=%d page=%d\r\n",
             addr7, size, page);
    UART_SendStringRing(UART_CMDLINE, buf);

    return CMDLINE_OK;
}

int Cmd_EEPROM_Write(int argc, char *argv[])
{
    if (argc > 4)
        return CMDLINE_INVALID_ARG;

    uint16_t addr = strtol(argv[1], NULL, 0);
    uint16_t len  = atoi(argv[2]);

    static uint8_t buf[256];

    if (len == 0 || len > sizeof(buf)) {
        UART_SendStringRing(UART_CMDLINE, "\r\nInvalid length\r\n");
        return CMDLINE_OK;
    }

    for (uint16_t i = 0; i < len; i++)
        buf[i] = i & 0xFF;

    if (EEPROM_Write(&EEPROM1, addr, buf, len) != HAL_OK) {
        UART_SendStringRing(UART_CMDLINE, "\r\nEEPROM write FAIL\r\n");
        return CMDLINE_OK;
    }

    UART_SendStringRing(UART_CMDLINE, "\r\nEEPROM write OK\r\n");
    return CMDLINE_OK;
}

int Cmd_EEPROM_Read(int argc, char *argv[])
{
    if (argc > 4)
        return CMDLINE_INVALID_ARG;

    uint16_t addr = strtol(argv[1], NULL, 0);
    uint16_t len  = atoi(argv[2]);

    static uint8_t buf[256];
    char out[64];

    if (len == 0 || len > sizeof(buf)) {
        UART_SendStringRing(UART_CMDLINE, "\r\nInvalid length\r\n");
        return CMDLINE_OK;
    }

    if (EEPROM_Read(&EEPROM1, addr, buf, len) != HAL_OK) {
        UART_SendStringRing(UART_CMDLINE, "\r\nEEPROM read FAIL\r\n");
        return CMDLINE_OK;
    }

    snprintf(out, sizeof(out), "\r\nEEPROM read @0x%04X (%d bytes):\r\n", addr, len);
    UART_SendStringRing(UART_CMDLINE, out);

    for (uint16_t i = 0; i < len; i++) {
        snprintf(out, sizeof(out), "%02X ", buf[i]);
        UART_SendStringRing(UART_CMDLINE, out);
        if ((i + 1) % 16 == 0)
            UART_SendStringRing(UART_CMDLINE, "\r\n");
    }
    UART_SendStringRing(UART_CMDLINE, "\r\n");

    return CMDLINE_OK;
}

int Cmd_EEPROM_Fill(int argc, char *argv[])
{
    if (argc > 4)
        return CMDLINE_INVALID_ARG;

    uint16_t addr = strtol(argv[1], NULL, 0);
    uint16_t len  = atoi(argv[2]);

    if (EEPROM_Fill(&EEPROM1, addr, len) != HAL_OK) {
        UART_SendStringRing(UART_CMDLINE, "\r\nEEPROM fill FAIL\r\n");
        return CMDLINE_OK;
    }

    UART_SendStringRing(UART_CMDLINE, "\r\nEEPROM fill OK\r\n");
    return CMDLINE_OK;
}


