/*
 * can_cmd.c
 *
 *  Created on: Feb 16, 2026
 *      Author: Samy Ve
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cmd.h"
#include "can_driver.h"
#include "FreeRTOS.h"
#include "task.h"

static int Cmd_CAN_Send_Buffer(int argc, char *argv[]) {
	if (argc > 2) return CMDLINE_TOO_MANY_ARGS;

	CAN_SendBuffer();
	return CMDLINE_OK;
}

static int Cmd_CAN_Read_Buffer(int argc, char *argv[]) {
	if (argc != 2)
	    return (argc < 2) ? CMDLINE_TOO_FEW_ARGS : CMDLINE_TOO_MANY_ARGS;

    Console_Write("DUT ID: 0x%x\r\n", can_driver.rx_header.StdId);
	Console_Write("Data:\r\n");

    char buffer[16];
    for (uint32_t i = 0; i < can_driver.rx_index; i++) { 
        sprintf(buffer, "%02X ", can_driver.rx_buffer[i]);
        Console_Write(buffer);
		if ((i + 1) % 16 == 0) {
			Console_Write("\r\n");
    	}
	}
    Console_Write("\r\n");
    memset(can_driver.rx_buffer, 0, sizeof(can_driver.rx_buffer));
    can_driver.rx_index = 0;
    return CMDLINE_OK;
}

// static int Cmd_CAN_Clear_Buffer(int argc, char *argv[]) {
// 	if (argc > 2) return CMDLINE_TOO_MANY_ARGS;

// 	memset(can_driver.rx_buffer, 0, sizeof(can_driver.rx_buffer));
// 	return CMDLINE_OK;
// }

// static int Cmd_CAN_LED(int argc, char *argv[]) {
// 	if (argc < 4) {
// 		return CMDLINE_TOO_FEW_ARGS;
// 	}
// 	if (argc > 4) {
// 		return CMDLINE_TOO_MANY_ARGS;
// 	}

// 	uint8_t tx_buf[2] = {0};
// 	tx_buf[0] = atoi(argv[1]); // LED status
// 	tx_buf[1] = atoi(argv[2]); // LED index

// 	if (tx_buf[0] > 1) {
// 		Console_Write("LED status must be 0 (OFF) or 1 (ON)\r\n");
// 		return CMDLINE_OK;
// 	}

// 	if (tx_buf[1] < 8 || tx_buf[1] > 10) {
// 		Console_Write("LED index must be between 8 and 10\r\n");
// 		return CMDLINE_OK;
// 	}

// 	CAN_Send(tx_buf, 2);

// 	vTaskDelay(pdMS_TO_TICKS(10));
// 	if (tx_buf[1] == 9) {
// 		if (LL_GPIO_IsInputPinSet(GPIOD, LL_GPIO_PIN_9)) {
// 			// LED_9 on
// 			Console_Write("LED_9 ON\r\n");
// 		} else {
// 			// LED_9 off
// 			Console_Write("LED_9 OFF\r\n");
// 		}
// 	} else if (tx_buf[1] == 10) {
// 		if (LL_GPIO_IsInputPinSet(GPIOD, LL_GPIO_PIN_10)) {
// 			// LED_10 on
// 			Console_Write("LED_10 ON\r\n");
// 		} else {
// 			// LED_10 off
// 			Console_Write("LED_10 OFF\r\n");
// 		}
// 	}
// 	return CMDLINE_OK;
// }

static int Cmd_CAN_Send_String(int argc, char *argv[])
{
	if (argc < 3) return CMDLINE_TOO_FEW_ARGS;

	for (int i = 1; i < argc - 1; i++) {
		CAN_Send_String(argv[i]);
		if (i < (argc - 2)) {
			CAN_Send_String(" ");
		}
	}
	Console_Write("CAN string sent\r\n");
	return CMDLINE_OK;
}

void Cmd_CAN_Register(void) {
	CLI_RegisterCommand("can_send_buffer", Cmd_CAN_Send_Buffer, "Send 256 bytes of data over CAN");
	CLI_RegisterCommand("can_send_string", Cmd_CAN_Send_String, "Send a string over CAN");
	CLI_RegisterCommand("can_read", Cmd_CAN_Read_Buffer, "Read received CAN data");
	// CLI_RegisterCommand("can_clear_buffer", Cmd_CAN_Clear_Buffer, "Clear received CAN data buffer");
	// CLI_RegisterCommand("can_led", Cmd_CAN_LED, "Turn on/ off DUT LED via CAN | format: can_led <led_status> <led_index>");
}
