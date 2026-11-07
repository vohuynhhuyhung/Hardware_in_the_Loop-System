/*
 * ananlog_cmd.c
 *
 *  Created on: Mar 10, 2026
 *      Author: VyTran
 */

#include <analog.h>
#include <stdio.h>
#include <stdlib.h>
#include "cmd.h"

static int cmd_adc_read(int argc, char *argv[])
{
	if (argc != 2)
	    return (argc < 2) ? CMDLINE_TOO_FEW_ARGS : CMDLINE_TOO_MANY_ARGS;

	float v = adc_read_voltage();
	uint16_t meas = v * 1000;
	Console_Write("\r\n Measured: %u mV\r\n", meas);
	return CMDLINE_OK;
}

static int cmd_adc_read_pwm(int argc, char *argv[])
{
	if (argc != 2)
	    return (argc < 2) ? CMDLINE_TOO_FEW_ARGS : CMDLINE_TOO_MANY_ARGS;

	float v = adc_read_pwm_voltage();
	uint16_t meas = v * 1000;
	Console_Write("\r\n Measured: %u mV\r\n", meas);
	return CMDLINE_OK;
}

extern DAC_HandleTypeDef hdac;
static int Cmd_DAC_Set_Voltage(int argc, char *argv[])
{
	if (argc != 3)
	    return (argc < 3) ? CMDLINE_TOO_FEW_ARGS : CMDLINE_TOO_MANY_ARGS;

    char *input = argv[1];
    char *dot_pos = strchr(input, '.');

    // Nếu có dấu chấm, kiểm tra phần thập phân
    if (dot_pos != NULL) {
        // dot_pos + 1 là vị trí chữ số đầu tiên sau dấu chấm
        // strlen(dot_pos + 1) sẽ trả về số lượng chữ số sau dấu chấm
        if (strlen(dot_pos + 1) > 1) {
            Console_Write("Only 1 decimal place allowed (e.g., 1.2)\r\n");
            return CMDLINE_INVALID_ARG;
        }
    }

    float voltage = atof(input);
    if (voltage < 0.0f || voltage > 3.3f) {
        Console_Write("Voltage out of range (0.0 - 3.3)\r\n");
        return CMDLINE_INVALID_ARG;
    }

    uint32_t dac_value = (uint32_t)((voltage / 3.3f) * 4095);
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, dac_value);
    return CMDLINE_OK;
}

int Cmd_set_freq(int argc, char *argv[])
{
	if (argc != 3)
	    return (argc < 3) ? CMDLINE_TOO_FEW_ARGS : CMDLINE_TOO_MANY_ARGS;

	uint32_t freq = atoi(argv[1]);
	if (freq == 0 || freq > 10000) {
		Console_Write("\r\nInvalid frequency\r\n");
		return CMDLINE_INVALID_ARG;
	}

	set_flag_freq_set(true);
	set_freq(freq);
	return CMDLINE_OK;
}

int Cmd_get_freq(int argc, char *argv[])
{
	if (argc != 2)
	    return (argc < 2) ? CMDLINE_TOO_FEW_ARGS : CMDLINE_TOO_MANY_ARGS;

	if (!is_freq_set()) {
		Console_Write("\r\nSine wave frequency is not set\r\n");
		return CMDLINE_OK;
	}

	Console_Write("\r\nSine wave frequency is %u Hz\r\n", get_freq());
	return (CMDLINE_OK);
}

int Cmd_Sine_Wave(int argc, char *argv[])
{
    if (argc != 3)
        return (argc < 3) ? CMDLINE_TOO_FEW_ARGS : CMDLINE_TOO_MANY_ARGS;

    uint32_t tmp = atoi(argv[1]);
    switch(tmp)
    {
        case 0:
            set_flag_sine_wave_on(false);
            set_flag_freq_set(false);
            stop_gen_wave();
            Console_Write("\r\nStop Sine Wave\r\n");
            break;

        case 1:
            if (!is_freq_set())
            {
                Console_Write("\r\nFrequency is not set\r\n");
                return CMDLINE_EXEC_FAILED;
            }

            if (is_triangle_wave_on())
            {
                Console_Write("\r\nTriangle wave is already on\r\n");
                return CMDLINE_EXEC_FAILED;
            }

            set_flag_sine_wave_on(true);
            start_gen_wave(SINE_WAVE);
            Console_Write("\r\nStart Sine Wave\r\n");
            break;

        default:
            Console_Write("\r\nInvalid argument\r\n");
            return CMDLINE_INVALID_ARG;
    }

    return CMDLINE_OK;
}

int Cmd_Triangle_Wave(int argc, char *argv[])
{
    if (argc != 3)
        return (argc < 3) ? CMDLINE_TOO_FEW_ARGS : CMDLINE_TOO_MANY_ARGS;

    uint32_t tmp = atoi(argv[1]);
    switch(tmp)
    {
        case 0:
            set_flag_triangle_wave_on(false);
            set_flag_freq_set(false);
            stop_gen_wave();
            Console_Write("\r\nStop Triangle Wave\r\n");
            break;

        case 1:
            if (!is_freq_set())
            {
                Console_Write("\r\nFrequency is not set\r\n");
                return CMDLINE_EXEC_FAILED;
            }

            if (is_sine_wave_on())
            {
                Console_Write("\r\nSine wave is already on\r\n");
                return CMDLINE_EXEC_FAILED;
            }

            set_flag_triangle_wave_on(true);
            start_gen_wave(TRIANGLE_WAVE);
            Console_Write("\r\nStart Triangle Wave\r\n");
            break;

        default:
            Console_Write("\r\nInvalid argument\r\n");
            return CMDLINE_INVALID_ARG;
    }

    return CMDLINE_OK;
}

void Cmd_Analog_Register(void)
{
	CLI_RegisterCommand("adc_read", cmd_adc_read, "Read ADC value");
	CLI_RegisterCommand("adc_read_pwm", cmd_adc_read_pwm, "Read PWM voltage value");
    CLI_RegisterCommand("dac_set_voltage", Cmd_DAC_Set_Voltage, "Set DAC voltage | format: dac_set_voltage <X.Y> (0.0-3.3V)");
    CLI_RegisterCommand("set_freq",  Cmd_set_freq,  "set sine wave frequency <freq>");
	CLI_RegisterCommand("get_freq",  Cmd_get_freq,  "get sine wave frequency");
	CLI_RegisterCommand("sine_wave",  Cmd_Sine_Wave,  "sine-wave <status>");
	CLI_RegisterCommand("triangle_wave",  Cmd_Triangle_Wave,  "triangle-wave <status>");
}


