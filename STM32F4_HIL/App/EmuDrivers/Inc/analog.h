/*
 * adc.h
 *
 *  Created on: Mar 10, 2026
 *      Author: VyTran
 */

#ifndef EMUDRIVERS_INC_ANALOG_H_
#define EMUDRIVERS_INC_ANALOG_H_

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

enum wave_type {
	SINE_WAVE = 0,
	TRIANGLE_WAVE = 1
};

float adc_read_voltage(void);
int pwm_verify_voltage(uint8_t ch, uint16_t target_mv, uint16_t tolerance_mv);
float adc_read_pwm_voltage(void);

void cal_sin (void);
void calc_triangle(void);
bool is_freq_set(void);
bool is_sine_wave_on(void);
bool is_triangle_wave_on(void);

void set_flag_sine_wave_on(bool status);
void set_flag_triangle_wave_on(bool status);
void set_flag_freq_set(bool status);

uint32_t get_freq(void);
void set_freq(uint32_t sine_wave_freq);
void stop_gen_wave(void);
void start_gen_wave(enum wave_type type);

#endif /* EMUDRIVERS_INC_ANALOG_H_ */
