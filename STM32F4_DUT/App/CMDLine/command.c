/*
 * command.c
 *
 *  Created on: Nov 21, 2024
 *      Author: Samy Ve
 */

#include "command.h"
#include "cmdline.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_hal_adc.h"
#include <stdlib.h>
#include <stdio.h>
#include "usb.h"
#include "w25q_driver.h"
#include "sine_wave.h"
#include "i2c_driver/i2c_eeprom.h"
#include "i2c_driver/i2c_rtc.h"
#include "adc_fft.h"

extern I2C_HandleTypeDef hi2c3;
static I2C_EEPROM_t EEPROM1;

extern DAC_HandleTypeDef hdac;

extern ADC_HandleTypeDef hadc1;

extern TIM_HandleTypeDef htim2;

// Add history
// Add some utils to CommandLine
#define NAME_SHELL "DUT:~ "
#define KEY_ENTER '\r'       /* [enter] key */
#define KEY_BACKSPACE '\x7f' /* [backspace] key */

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

const char *ErrorCode[6] = {
	"OK\r\n",
	"BAD_CMD\r\n",
	"TOO_MANY_ARGS\r\n",
	"TOO_FEW_ARGS\r\n",
	"INVALID_ARG\r\n",
	"CMD_OK_BUT_PENDING...\r\n"
};

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

	{ "i2c_scan",     Cmd_I2C_Scan,     "Scan I2C device active on bus" },
	{ "eeprom_init",  Cmd_EEPROM_Init,  "eeprom_init <addr7bit> <size> <page>" },
	{ "eeprom_write", Cmd_EEPROM_Write, "eeprom_write <addr> <len>" },
	{ "eeprom_read",  Cmd_EEPROM_Read,  "eeprom_read <addr> <len>" },
	{ "eeprom_fill",  Cmd_EEPROM_Fill,  "eeprom_fill <addr> <len>" },

	{ "rtc_init",  Cmd_RTC_Init,  "Init the communication with RTC DS1307" },
	{ "rtc_set_time",  Cmd_RTC_Set_Time,  "rtc_set_time <hour> <min> <sec>" },
	{ "rtc_get_time",  Cmd_RTC_Get_Time,  "get RTC time" },
	{ "rtc_set_date",  Cmd_RTC_Set_Date,  "rtc_set_date <day> <date> <mon> <year>" },
	{ "rtc_get_date",  Cmd_RTC_Get_Date,  "get RTC date" },

	{ "set_freq",  Cmd_set_freq,  "set sine wave frequency <freq>" },
	{ "get_freq",  Cmd_get_freq,  "get sine wave frequency" },
	{ "sine_wave",  Cmd_Sine_Wave,  "sine-wave <status>" },
	{ "triangle_wave",  Cmd_Triangle_Wave,  "triangle-wave <status>" },

    { "uart_init",      Cmd_UART_Init,           "Initialize UART RX interrupt" },
    { "uart_dump",      Cmd_UART_Dump_Buffer,    "Transmit 256 incremental bytes" },
    { "uart_rx",        Cmd_UART_Receive, 		 "Received UART data" },
    { "uart_tx",        Cmd_UART_Send_String,    "uart_send <text>" },

	{ "adc_read",       Cmd_ADC_Read,            "Read ADC value | format: adc_read" },

	{ "pwm_set_freq",  Cmd_PWM_Freq,  "pwm_set_freq <freq_hz>" },
	{ "pwm_set_duty_cycle",  Cmd_PWM_Duty_Cycle,  "pwm_set_duty_cycle <ch> <0-100>" },
	{ "pwm_start", Cmd_PWM_Start, "pwm_start <ch>" },
	{ "pwm_stop",  Cmd_PWM_Stop,  "pwm_stop <ch>" },

	{ "set_led", Cmd_Set_Led, "set_led <led_idx> <0|1>" },

	{ "read_sine_freq", Cmd_Read_Sine_Frequency, "read_sine_freq" },

	{ 0, 0, 0 }
};

void CommandLine_Task(void *pvParameters)
{
	while (1) {
		CommandLine_Task_Update(); // gọi hàm xử lý CLI
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

void CommandLine_Init(void) {
	Console_Init();
	memset((void*) s_commandBuffer, 0, sizeof(s_commandBuffer));
	s_commandBufferIndex = 0;
	Console_Write("\n\n\rDUT FIRMWARE \r\n");

	Console_Write(NAME_SHELL);
}

static void CommandLine_Task_Update(void) {
	char rxData;
	if (Console_Available()) {
		rxData = Console_Read();
		if (rxData == 27) {
			Console_Write("\033[2J\033[H");
			Console_Write("\r\n");
			Console_Write(NAME_SHELL);
		} else {
			// Echo ra terminal
			char tmp[2] = {rxData, 0};
			Console_Write(tmp);
		}
		process_command(rxData, &pContext);
	}
}

void process_command(char rxData, CMDLine_Context *context) {
	if (rxData == 0x2D) { // '-' key (history up)
		if (context->historyIndex > 0) {
			context->historyIndex--;
		}
		// Load history command
		if (context->historyIndex < context->historyCount) {
			strcpy(context->commandBuffer, context->commandHistory[context->historyIndex]);
			context->commandBufferIndex = strlen(context->commandBuffer);
		} else {
			context->commandBuffer[0] = '\0';
			context->commandBufferIndex = 0;
		}
		// Clear current line and display updated command
		Console_Write("\033[2K"); // Clear entire line
		Console_Write("\r\n");
		Console_Write(NAME_SHELL);
		Console_Write(context->commandBuffer); // Display updated command
		return;
	} else if (rxData == 0x3D) { // '=' key (history down)
		if (context->historyIndex < context->historyCount) {
			context->historyIndex++;
		}
		// Load history command
		if (context->historyIndex < context->historyCount) {
			strcpy(context->commandBuffer, context->commandHistory[context->historyIndex]);
			context->commandBufferIndex = strlen(context->commandBuffer);
		} else {
			context->commandBuffer[0] = '\0';
			context->commandBufferIndex = 0;
		}
		// Clear current line and display updated command
		Console_Write("\033[2K"); // Clear entire line
		Console_Write("\r\n");
		Console_Write(NAME_SHELL);
		Console_Write(context->commandBuffer); // Display updated command
		return;
	}

	// Handle individual key presses
	if (((rxData >= 32 && rxData <= 126) || rxData == KEY_ENTER || rxData == KEY_BACKSPACE)
	    && rxData != 0x2D && rxData != 0x3D && rxData != 0x5C) {
		if (rxData == KEY_ENTER) {
			if (context->commandBufferIndex > 0) {
				context->commandBuffer[context->commandBufferIndex] = '\0';
				// Save to history
				if (context->historyCount == 0 || strcmp(context->commandHistory[context->historyCount-1], context->commandBuffer) != 0) {
					if (context->historyCount < MAX_HISTORY) {
						strcpy(context->commandHistory[context->historyCount], context->commandBuffer);
						context->historyCount++;
					} else {
						for (int i = 0; i < MAX_HISTORY - 1; i++) {
							strcpy(context->commandHistory[i], context->commandHistory[i + 1]);
						}
						strcpy(context->commandHistory[MAX_HISTORY - 1], context->commandBuffer);
					}
				}
				context->historyIndex = context->historyCount;

				// Process command
				int8_t ret_val = CmdLineProcess(context->commandBuffer);
				Console_Write("\r\n");
				Console_Write(ErrorCode[ret_val]);
				Console_Write("\r\n");
				Console_Write(NAME_SHELL);
				context->commandBufferIndex = 0;
		        memset(context->commandBuffer, 0, sizeof(context->commandBuffer));
			} else {
				Console_Write("\r\n");
				Console_Write(NAME_SHELL);
			}
		} else if (rxData == KEY_BACKSPACE) {
			if (context->commandBufferIndex > 0) {
				context->commandBufferIndex--;
				context->commandBuffer[context->commandBufferIndex] = '\0';
			} else {
				Console_Write(" ");
			}
		} else {
			if (context->commandBufferIndex < COMMAND_MAX_LENGTH - 1) {
				context->commandBuffer[context->commandBufferIndex++] = rxData;
				context->commandBuffer[context->commandBufferIndex] = '\0';
			} else {
				// Command too long
				Console_Write("\r\nError: Command too long.");
				Console_Write("\r\n");
				Console_Write(NAME_SHELL);
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
		Console_Write(buffer);
		pEntry++;
	}
	return (CMDLINE_OK);
}

int Cmd_Clear_CLI(int argc, char *argv[]) {
	if (argc > 2) return CMDLINE_TOO_MANY_ARGS;
	Console_Write("\033[2J\033[H");
	return (CMDLINE_OK);
}

int Cmd_ReadID_W25Q(int argc, char *argv[]) {
	if (argc > 2) return CMDLINE_TOO_MANY_ARGS;
	uint8_t id[4];
	char buffer[30];
	W25Q_read_id(&W25Q, id);
	snprintf(buffer, sizeof(buffer), "\r\nW25Q ID: 0x%X%X%X%x\r\n", id[0], id[1], id[2], id[3]);
	Console_Write(buffer);
	return (CMDLINE_OK);
}

int Cmd_Write_W25Q(int argc, char *argv[]) {
	if (argc < 3)
		return CMDLINE_TOO_FEW_ARGS;
	if (argc > 3)
		return CMDLINE_TOO_MANY_ARGS;

	char buffer[64];
	uint32_t length = atoi(argv[1]); // số byte cần ghi

	if (length == 0 || length > 1024) { // Giới hạn để tránh buffer quá lớn
		Console_Write("\r\nInvalid length\r\n");
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
	Console_Write(buffer);

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
		Console_Write("\r\nInvalid length\r\n");
		return CMDLINE_OK;
	}

	// Tạo buffer để đọc dữ liệu
	static uint8_t read_data[1024]; // đủ lớn

	// Đọc từ flash
	W25Q_read(&W25Q, 0x000000, length, read_data);

	// Hiển thị dữ liệu đọc được
	snprintf(buffer, sizeof(buffer), "\r\nRead %lu bytes from addr [0 - %lu] from W25Q:\r\n",
	         (unsigned long)length, (unsigned long)(length - 1));
	Console_Write(buffer);
	for (uint32_t i = 0; i < length; i++) {
		snprintf(buffer, sizeof(buffer), "%02X ", read_data[i]);
		Console_Write(buffer);
		if ((i + 1) % 16 == 0) {
			Console_Write("\r\n");
		}
	}
	Console_Write("\r\n");

	return CMDLINE_OK;
}

int Cmd_EraseChip_W25Q(int argc, char *argv[]) {
	if (argc > 2)
		return CMDLINE_TOO_MANY_ARGS;

	char buffer[30];
	// Xóa chip
	W25Q_chip_erase(&W25Q);
	snprintf(buffer, sizeof(buffer), "\r\nChip Erase OK\r\n");
	Console_Write(buffer);

	return CMDLINE_OK;
}

int Cmd_EEPROM_Init(int argc, char *argv[])
{
	if (argc > 5)
		return CMDLINE_INVALID_ARG;

	uint8_t addr7 = strtol(argv[1], NULL, 0);
	uint16_t size  = atoi(argv[2]);
	uint16_t page  = atoi(argv[3]);

	if (EEPROM_Init(&EEPROM1,
	                &hi2c3,
	                addr7,
	                size,
	                page,
	                I2C_MEMADD_SIZE_16BIT) != HAL_OK) {
		Console_Write("\r\nEEPROM init FAIL\r\n");
		return CMDLINE_OK;
	}

	char buf[64];
	snprintf(buf, sizeof(buf),
	         "\r\nEEPROM init OK: addr=0x%02X size=%d page=%d\r\n",
	         addr7, size, page);
	Console_Write(buf);

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
		Console_Write("\r\nInvalid length\r\n");
		return CMDLINE_OK;
	}

	for (uint16_t i = 0; i < len; i++)
		buf[i] = i & 0xFF;

	if (EEPROM_Write(&EEPROM1, addr, buf, len) != HAL_OK) {
		Console_Write("\r\nEEPROM write FAIL\r\n");
		return CMDLINE_OK;
	}

	Console_Write("\r\nEEPROM write OK\r\n");
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
		Console_Write("\r\nInvalid length\r\n");
		return CMDLINE_OK;
	}

	if (EEPROM_Read(&EEPROM1, addr, buf, len) != HAL_OK) {
		Console_Write("\r\nEEPROM read FAIL\r\n");
		return CMDLINE_OK;
	}

	snprintf(out, sizeof(out), "\r\nEEPROM read @0x%04X (%d bytes):\r\n", addr, len);
	Console_Write(out);

	for (uint16_t i = 0; i < len; i++) {
		snprintf(out, sizeof(out), "%02X ", buf[i]);
		Console_Write(out);
		if ((i + 1) % 16 == 0)
			Console_Write("\r\n");
	}
	Console_Write("\r\n");

	return CMDLINE_OK;
}

int Cmd_EEPROM_Fill(int argc, char *argv[])
{
	if (argc > 4)
		return CMDLINE_INVALID_ARG;

	uint16_t addr = strtol(argv[1], NULL, 0);
	uint16_t len  = atoi(argv[2]);

	if (EEPROM_Fill(&EEPROM1, addr, len) != HAL_OK) {
		Console_Write("\r\nEEPROM fill FAIL\r\n");
		return CMDLINE_OK;
	}

	Console_Write("\r\nEEPROM fill OK\r\n");
	return CMDLINE_OK;
}

int Cmd_RTC_Init(int argc, char *argv[])
{
	if (argc < 2)
		return CMDLINE_TOO_FEW_ARGS;
	if (argc > 2)
		return CMDLINE_TOO_MANY_ARGS;

	//Địa chỉ của DS1307 được để mặc định là 0x68 nên không cần set
	DS1307_Init(&hi2c3);
	Console_Write("\r\nDS1307 init OK\r\n");
	return CMDLINE_OK;
}

int Cmd_RTC_Set_Time(int argc, char *argv[])
{
	if (argc < 5)
		return CMDLINE_TOO_FEW_ARGS;
	if (argc > 5)
		return CMDLINE_TOO_MANY_ARGS;

    uint8_t hour = atoi(argv[1]);
    uint8_t min  = atoi(argv[2]);
    uint8_t sec  = atoi(argv[3]);

    if (hour > 23 || min > 59 || sec > 59)
    {
        Console_Write("ERR: Invalid time\r\n");
        return CMDLINE_OK;
    }

    DS1307_SetHour(hour);
    DS1307_SetMinute(min);
    DS1307_SetSecond(sec);

    Console_Write("DS1307 set time OK: %02d:%02d:%02d\r\n",
                  hour, min, sec);

    return CMDLINE_OK;
}

int Cmd_RTC_Get_Time(int argc, char *argv[])
{
	if (argc < 2)
		return CMDLINE_TOO_FEW_ARGS;
	if (argc > 2)
		return CMDLINE_TOO_MANY_ARGS;

	uint8_t hour = DS1307_GetHour();
	uint8_t min = DS1307_GetMinute();
	uint8_t sec = DS1307_GetSecond();
	Console_Write("\r\nDS1307 get time OK: %d:%d:%d\r\n", hour, min, sec);
	return CMDLINE_OK;
}

int Cmd_RTC_Set_Date(int argc, char *argv[])
{
	if (argc < 6)
		return CMDLINE_TOO_FEW_ARGS;
	if (argc > 6)
		return CMDLINE_TOO_MANY_ARGS;

    uint8_t day   = atoi(argv[1]);
    uint8_t date  = atoi(argv[2]);
    uint8_t month = atoi(argv[3]);
    uint8_t year2 = atoi(argv[4]);   // 0..99

    if (year2 > 99)
    {
        Console_Write("ERR: Invalid year (0-99)\r\n");
        return CMDLINE_OK;
    }

    uint16_t full_year = 2000 + year2;

    if (!rtc_validate_date(day, date, month, full_year))
    {
        Console_Write("ERR: Invalid date\r\n");
        return CMDLINE_OK;
    }

    DS1307_SetDayOfWeek(day);
    DS1307_SetDate(date);
    DS1307_SetMonth(month);
    DS1307_SetYear(full_year);

    Console_Write("DS1307 set date OK: DOW=%d %02d/%02d/%04d\r\n",
                  day, date, month, full_year);

    return CMDLINE_OK;
}

int Cmd_RTC_Get_Date(int argc, char *argv[])
{
	if (argc < 2)
		return CMDLINE_TOO_FEW_ARGS;
	if (argc > 2)
		return CMDLINE_TOO_MANY_ARGS;

    uint8_t day   = DS1307_GetDayOfWeek();
    uint8_t date  = DS1307_GetDate();
    uint8_t month = DS1307_GetMonth();
    uint16_t year = DS1307_GetYear();

    Console_Write("DS1307 get date OK: DOW=%d %02d/%02d/%04d\r\n",
                  day, date, month, year);

    return CMDLINE_OK;
}

int Cmd_I2C_Scan(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	char msg[64];
	uint8_t found = 0;

	Console_Write("\r\nScanning I2C bus...\r\n");

	for (uint8_t addr = 1; addr < 127; addr++) {
		if (HAL_I2C_IsDeviceReady(&hi2c3, addr << 1, 2, 10) == HAL_OK) {
			sprintf(msg, "Found device at 0x%02X\r\n", addr);
			Console_Write(msg);
			found++;
		}
	}

	if (found == 0)
		Console_Write("No I2C device found\r\n");
	else {
		sprintf(msg, "Total: %d device(s)\r\n", found);
		Console_Write(msg);
	}

	return CMDLINE_OK;
}

int Cmd_set_freq(int argc, char *argv[]) {
	if (argc < 3) {
		return CMDLINE_TOO_FEW_ARGS;
	}
	if (argc > 3) {
		return CMDLINE_TOO_MANY_ARGS;
	}

	uint32_t freq = atoi(argv[1]);
	if (freq == 0 || freq > 10000) {
		Console_Write("\r\nInvalid frequency\r\n");
		return CMDLINE_OK;
	}
	set_flag_freq_set(true);
	set_freq(freq);
	return CMDLINE_OK;
}


int Cmd_get_freq(int argc, char *argv[]) {
	if (argc > 2) return CMDLINE_TOO_MANY_ARGS;

	if (!is_freq_set()) {
		Console_Write("\r\nSine wave frequency is not set\r\n");
		return CMDLINE_OK;
	}
	Console_Write("\r\nSine wave frequency is %u Hz\r\n", get_freq());
	return (CMDLINE_OK);
}

int Cmd_Sine_Wave(int argc, char *argv[]) {
	if (argc < 3)
		return CMDLINE_TOO_FEW_ARGS;
	if (argc > 3)
		return CMDLINE_TOO_MANY_ARGS;

	if (!is_freq_set()) {
		Console_Write("\r\nFrequency is not set\r\n");
		return CMDLINE_OK;
	}

	uint32_t tmp = atoi(argv[1]); // số byte cần ghi

	if (tmp == 1) {
		if (is_triangle_wave_on()) {
			Console_Write("\r\nTriangle wave is already on\r\n");
			return CMDLINE_OK;
		}
		set_flag_sine_wave_on(true);
		start_gen_wave(SINE_WAVE);
		Console_Write("\r\nStart Sine Wave\r\n");
		return CMDLINE_OK;
	} else if (tmp == 0) {
		set_flag_sine_wave_on(false);
		set_flag_freq_set(false);
		stop_gen_wave();
		Console_Write("\r\nStop Sine Wave\r\n");
		return CMDLINE_OK;
	} else {
		Console_Write("\r\nInvalid argument\r\n");
		return CMDLINE_OK;
	}
}

int Cmd_Triangle_Wave(int argc, char *argv[]) {
	if (argc < 3)
		return CMDLINE_TOO_FEW_ARGS;
	if (argc > 3)
		return CMDLINE_TOO_MANY_ARGS;

	if (!is_freq_set()) {
		Console_Write("\r\nFrequency is not set\r\n");
		return CMDLINE_OK;
	}

	uint32_t tmp = atoi(argv[1]); // số byte cần ghi

	if (tmp == 1) {
		if (is_sine_wave_on()) {
			Console_Write("\r\nSine wave is already on\r\n");
			return CMDLINE_OK;
		}
		set_flag_triangle_wave_on(true);
		start_gen_wave(TRIANGLE_WAVE);
		Console_Write("\r\nStart Triangle Wave\r\n");
		return CMDLINE_OK;
	} else if (tmp == 0) {
		set_flag_triangle_wave_on(false);
		set_flag_freq_set(false);
		stop_gen_wave();
		Console_Write("\r\nStop Triangle Wave\r\n");
		return CMDLINE_OK;
	} else {
		Console_Write("\r\nInvalid argument\r\n");
		return CMDLINE_OK;
	}
}

// UART
#include "uart_driver.h"

int Cmd_UART_Init(int argc, char *argv[])
{
	if (argc != 2)
	    return (argc < 2) ? CMDLINE_TOO_FEW_ARGS : CMDLINE_TOO_MANY_ARGS;

    UART_Init();
    UART_Clear();
    Console_Write("\r\nUART Initialized\r\n");
    return CMDLINE_OK;
}

int Cmd_UART_Dump_Buffer(int argc, char *argv[])
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

int Cmd_UART_Receive(int argc, char *argv[])
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

int Cmd_UART_Send_String(int argc, char *argv[])
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

int Cmd_ADC_Read(int argc, char *argv[])
{
    if (argc > 2) return CMDLINE_TOO_MANY_ARGS;

    HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, 100);
	uint16_t adc_value = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_Stop(&hadc1);

	uint32_t voltage = (adc_value * 3300) / 4095;
	Console_Write("\r\nVoltage: %lu mV\r\n", voltage);
    return CMDLINE_OK;
}

#include "../pwm/pwm_control.h"

int Cmd_PWM_Freq(int argc, char *argv[])
{
	if (argc < 3) return CMDLINE_TOO_FEW_ARGS;
	if (argc > 3) return CMDLINE_TOO_MANY_ARGS;

	uint32_t freq = atoi(argv[1]);

	if(freq == 0)
	{
		Console_Write("\r\nInvalid freq\r\n");
		return CMDLINE_OK;
	}

	set_pwm_freq(freq);
	Console_Write("\r\nPWM freq set to %d Hz\r\n", freq);

	return CMDLINE_OK;
}

int Cmd_PWM_Duty_Cycle(int argc, char *argv[])
{
	if (argc < 4) return CMDLINE_TOO_FEW_ARGS;
	if (argc > 4) return CMDLINE_TOO_MANY_ARGS;

	uint8_t ch = atoi(argv[1]);
	uint8_t duty = atoi(argv[2]);

	if(ch <1 || ch > 4)
	{
		Console_Write("\r\nInvalid channel (1-4)\r\n");
		return CMDLINE_OK;
	}

	set_pwm_duty_cycle(ch, duty);
	Console_Write("\r\nCH%d duty cycle set to %u %\r\n",ch, duty);

	return CMDLINE_OK;
}

int Cmd_PWM_Start(int argc, char *argv[])
{
	if (argc < 3) return CMDLINE_TOO_FEW_ARGS;
	if (argc > 3) return CMDLINE_TOO_MANY_ARGS;

	uint8_t ch = atoi(argv[1]);

	if(pwm_start(ch))
	{
		Console_Write("\r\nInvalid channel\r\n");
		return CMDLINE_OK;
	}

	Console_Write("\r\nPWM CH%d started\r\n",ch);
	return CMDLINE_OK;
}

int Cmd_PWM_Stop(int argc, char *argv[])
{
	if (argc < 3) return CMDLINE_TOO_FEW_ARGS;
	if (argc > 3) return CMDLINE_TOO_MANY_ARGS;

	uint8_t ch = atoi(argv[1]);

	if(pwm_stop(ch))
	{
		Console_Write("\r\nInvalid channel\r\n");
		return CMDLINE_OK;
	}

	Console_Write("\r\nPWM CH%d stopped\r\n",ch);
	return CMDLINE_OK;
}

int Cmd_Set_Led(int argc, char *argv[])
{
	if (argc < 4) return CMDLINE_TOO_FEW_ARGS;
	if (argc > 4) return CMDLINE_TOO_MANY_ARGS;

	uint8_t led_index = atoi(argv[1]);
	uint8_t state = atoi(argv[2]);

	if (led_index < 1 || led_index > 5) {
		Console_Write("\r\nLED index out of bounds (1-5)\r\n");
		return CMDLINE_OK;
	}

	if (state != 0 && state != 1) {
		Console_Write("\r\nInvalid state. Please use 0 (OFF) or 1 (ON)\r\n");
		return CMDLINE_OK;
	}
	

	if (state == 1) {
			// Bật LED tương ứng
			switch (led_index) {
			case 1:
				LL_GPIO_SetOutputPin(GPIOD, LL_GPIO_PIN_1);
				break;
			case 2:
				LL_GPIO_SetOutputPin(GPIOD, LL_GPIO_PIN_2);
				break;
			case 3:
				LL_GPIO_SetOutputPin(GPIOD, LL_GPIO_PIN_3);
				break;
			case 4:
				LL_GPIO_SetOutputPin(GPIOD, LL_GPIO_PIN_4);
				break;
			case 5:
				LL_GPIO_SetOutputPin(GPIOD, LL_GPIO_PIN_5);
				break;
			default:
				break;
			}
		} else {
			// Tắt LED tương ứng
			switch (led_index) {
			case 1:
				LL_GPIO_ResetOutputPin(GPIOD, LL_GPIO_PIN_1);
				break;
			case 2:
				LL_GPIO_ResetOutputPin(GPIOD, LL_GPIO_PIN_2);
				break;
			case 3:
				LL_GPIO_ResetOutputPin(GPIOD, LL_GPIO_PIN_3);
				break;
			case 4:
				LL_GPIO_ResetOutputPin(GPIOD, LL_GPIO_PIN_4);
				break;
			case 5:
				LL_GPIO_ResetOutputPin(GPIOD, LL_GPIO_PIN_5);
				break;
			default:
				break;
			}
		}

	return CMDLINE_OK;
}

int Cmd_Read_Sine_Frequency(int argc, char *argv[])
{
	if (argc > 2) return CMDLINE_TOO_MANY_ARGS;

	ADC_FFT_TriggerCapture();

	return CMDLINE_OK;
}
