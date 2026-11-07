/*
 * dut_control_cmd.c
 *
 *  Created on: Mar 16, 2026
 *      Author: Vy Tran
 */

#ifndef CMDLINE_COMMANDS_DUT_CONTROL_CMD_C_
#define CMDLINE_COMMANDS_DUT_CONTROL_CMD_C_

#include <stdio.h>
#include <stdlib.h>
#include "cmd.h"
#include <stm32f4xx_ll_gpio.h>
#include "FreeRTOS.h"
#include "w25q_slave.h"

static int Cmd_Power_DUT(int argc, char *argv[])
{
    if (argc != 3)
        return (argc < 3) ? CMDLINE_TOO_FEW_ARGS : CMDLINE_TOO_MANY_ARGS;

    uint32_t status = atoi(argv[1]);
    switch(status)
    {
        case 1:
            LL_GPIO_SetOutputPin(DUT_POWER_GPIO_Port, DUT_POWER_Pin);

            vTaskDelay(pdMS_TO_TICKS(100));
            SPI3_Reset();

            if (LL_GPIO_IsInputPinSet(DUT_POWER_STATUS_GPIO_Port,
                                      DUT_POWER_STATUS_Pin)) {
                Console_Write("DUT is powered ON\r\n");
            } else {
                Console_Write("Failed to power ON DUT\r\n");
                return CMDLINE_EXEC_FAILED;
            }
            break;

        case 0:
            LL_GPIO_ResetOutputPin(DUT_POWER_GPIO_Port, DUT_POWER_Pin);

            vTaskDelay(pdMS_TO_TICKS(100));

            if (LL_GPIO_IsInputPinSet(DUT_POWER_STATUS_GPIO_Port,
                                      DUT_POWER_STATUS_Pin)) {
                Console_Write("Failed to power OFF DUT\r\n");
                return CMDLINE_EXEC_FAILED;
            } else {
                Console_Write("DUT is powered OFF\r\n");
            }
            break;

        default:
            return CMDLINE_INVALID_ARG;
    }

    return CMDLINE_OK;
}

static int Cmd_Power_Status(int argc, char *argv[])
{
	if (argc != 2)
	    return (argc < 2) ? CMDLINE_TOO_FEW_ARGS : CMDLINE_TOO_MANY_ARGS;

    if (LL_GPIO_IsInputPinSet(DUT_POWER_STATUS_GPIO_Port, DUT_POWER_STATUS_Pin)) {
        // DUT đã bật (logic HIGH)
        Console_Write("DUT ON\r\n");
    } else {
        // DUT không bật được (logic LOW)
        Console_Write("DUT OFF\r\n");
    }

    return CMDLINE_OK;
}

static int Cmd_Set_BOOT0(int argc, char *argv[])
{
	if (argc != 3)
	    return (argc < 3) ? CMDLINE_TOO_FEW_ARGS : CMDLINE_TOO_MANY_ARGS;

    uint32_t status = atoi(argv[1]);
    switch (status) {
		case 0:
	        LL_GPIO_ResetOutputPin(DUT_BOOT0_GPIO_Port, DUT_BOOT0_Pin);

	        vTaskDelay(pdMS_TO_TICKS(100));

	        if (!LL_GPIO_IsOutputPinSet(DUT_BOOT0_GPIO_Port, DUT_BOOT0_Pin)) {
	        	Console_Write("DUT BOOT0 set to low successfully \r\n");
	        } else {
	        	Console_Write("FAIL to set DUT BOOT0 to low \r\n");
	            return CMDLINE_EXEC_FAILED;
	        }

			break;
		case 1:
	        LL_GPIO_SetOutputPin(DUT_BOOT0_GPIO_Port, DUT_BOOT0_Pin);

	        vTaskDelay(pdMS_TO_TICKS(100));

	        if (LL_GPIO_IsOutputPinSet(DUT_BOOT0_GPIO_Port, DUT_BOOT0_Pin)) {
	        	Console_Write("DUT BOOT0 set to high successfully \r\n");
	        } else {
	        	Console_Write("FAIL to set DUT BOOT0 to high \r\n");
	            return CMDLINE_EXEC_FAILED;
	        }

			break;
		default:
			return CMDLINE_INVALID_ARG;
	}

	return CMDLINE_OK;
}

void Cmd_DUT_Control_Register(void)
{
	CLI_RegisterCommand("dut_power", Cmd_Power_DUT, "Power DUT on/off | format: dut_power <0|1>");
	CLI_RegisterCommand("dut_power_status", Cmd_Power_Status, "Check DUT power status | format: dut_power_status");
	CLI_RegisterCommand("dut_boot0_set", Cmd_Set_BOOT0, "Set DUT BOOT0 status | format: dut_boot0_set <0|1>");
}

#endif /* CMDLINE_COMMANDS_DUT_CONTROL_CMD_C_ */
