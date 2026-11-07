/*
 * pwm_cmd.c
 *
 *  Created on: Apr 3, 2026
 *      Author: Vy Tran
 */

#include <stdio.h>
#include <stdlib.h>
#include "cmd.h"
#include "pwm_control.h"

int Cmd_PWM_Freq(int argc, char *argv[])
{
	if (argc != 3)
	    return (argc < 3) ? CMDLINE_TOO_FEW_ARGS : CMDLINE_TOO_MANY_ARGS;

	uint32_t freq = atoi(argv[1]);

	if(freq == 0)
	{
		Console_Write("\r\nInvalid freq\r\n");
		return CMDLINE_INVALID_ARG;
	}

	set_pwm_freq(freq);
	Console_Write("\r\nPWM freq set to %d Hz\r\n", freq);
	return CMDLINE_OK;
}

int Cmd_PWM_Duty_Cycle(int argc, char *argv[])
{
	if (argc != 4)
	    return (argc < 4) ? CMDLINE_TOO_FEW_ARGS : CMDLINE_TOO_MANY_ARGS;

	uint8_t ch = atoi(argv[1]);
	uint8_t duty = atoi(argv[2]);

	// if(ch < 2 || ch > 4)
	// {
	// 	Console_Write("\r\nInvalid channel (2-4)\r\n");
	// 	return CMDLINE_INVALID_ARG;
	// }

	set_pwm_duty_cycle(ch, duty);
	Console_Write("\r\nCH%d duty cycle set to %u %\r\n",ch, duty);
	return CMDLINE_OK;
}

int Cmd_PWM_Start(int argc, char *argv[])
{
	if (argc != 3)
	    return (argc < 3) ? CMDLINE_TOO_FEW_ARGS : CMDLINE_TOO_MANY_ARGS;

	uint8_t ch = atoi(argv[1]);
	if(pwm_start(ch))
	{
		Console_Write("\r\nInvalid channel\r\n");
		return CMDLINE_INVALID_ARG;
	}

	Console_Write("\r\nPWM CH%d started\r\n",ch);
	return CMDLINE_OK;
}

int Cmd_PWM_Stop(int argc, char *argv[])
{
	if (argc != 3)
	    return (argc < 3) ? CMDLINE_TOO_FEW_ARGS : CMDLINE_TOO_MANY_ARGS;

	uint8_t ch = atoi(argv[1]);
	if(pwm_stop(ch))
	{
		Console_Write("\r\nInvalid channel\r\n");
		return CMDLINE_INVALID_ARG;
	}

	Console_Write("\r\nPWM CH%d stopped\r\n",ch);
	return CMDLINE_OK;
}

void Cmd_PWM_Register(void)
{
	CLI_RegisterCommand("pwm_start", Cmd_PWM_Start, "pwm_start <ch>");
	CLI_RegisterCommand("pwm_stop", Cmd_PWM_Stop, "pwm_stop <ch>");
	CLI_RegisterCommand("pwm_set_freq", Cmd_PWM_Freq, "pwm_set_freq <freq_hz>");
	CLI_RegisterCommand("pwm_set_duty_cycle", Cmd_PWM_Duty_Cycle, "pwm_set_duty_cycle <ch> <0-100>");
}
