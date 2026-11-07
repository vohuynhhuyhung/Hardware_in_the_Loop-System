/*
 * sine_wave.c
 *
 *  Created on: Feb 14, 2026
 *      Author: Samy Ve
 */

#include "sine_wave.h"
#include "math.h"

#define PI 3.1415926
#define SAMPLES_NUM 100
#define DAC_MAX 4094
#define APB2_CLK 60000000
#define ARR_MAX   0xFFFFFFFFULL

extern DAC_HandleTypeDef hdac;

extern TIM_HandleTypeDef htim2;

uint32_t sine_val[100];
uint32_t triangle_val[100];

struct sine_wave_t
{
	bool is_freq_set;
	bool is_sine_wave_on;
	bool is_triangle_wave_on;
	uint32_t freq;

};

struct sine_wave_t sine_wave = {
	.is_freq_set = false,
	.is_sine_wave_on = false,
	.is_triangle_wave_on = false,
	.freq = 0
};

void calc_triangle(void) {
	for (uint32_t i = 0; i < SAMPLES_NUM; i++) {
		if (i < SAMPLES_NUM/2) {
			triangle_val[i] = (2 * DAC_MAX * i) / SAMPLES_NUM;
		} else {
			triangle_val[i] = (2 * DAC_MAX * (SAMPLES_NUM - i)) / SAMPLES_NUM;
		}
	}
}

void calcsin (void) {
	for (uint32_t i=0; i<SAMPLES_NUM; i++) {
		sine_val[i] = ((sin(i*2*PI/SAMPLES_NUM) + 1)*(DAC_MAX/2))*0.98;
	}
}

bool is_sine_wave_on(void) {
	return sine_wave.is_sine_wave_on;
}

bool is_triangle_wave_on(void) {
	return sine_wave.is_triangle_wave_on;
}

bool is_freq_set(void) {
	return sine_wave.is_freq_set;
}

void set_flag_sine_wave_on(bool status) {
	sine_wave.is_sine_wave_on = status;
}

void set_flag_triangle_wave_on(bool status) {
	sine_wave.is_triangle_wave_on = status;
}

void set_flag_freq_set(bool status) {
	sine_wave.is_freq_set = status;
}

void set_freq(uint32_t sine_wave_freq) {
	sine_wave.freq = sine_wave_freq;

	uint32_t f_sample = sine_wave_freq * SAMPLES_NUM;
	uint64_t psc = 0;
    uint64_t arr;

    arr = APB2_CLK / f_sample;

    while(arr > ARR_MAX)
    {
        psc++;
        arr = APB2_CLK / ((psc + 1) * f_sample);
    }

	htim2.Init.Prescaler = psc;
	htim2.Init.Period = arr - 1;
	HAL_TIM_Base_Init(&htim2);
}

uint32_t get_freq(void) {
	return sine_wave.freq;
}

void stop_gen_wave(void) {
	HAL_TIM_Base_Stop(&htim2);
	HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
}

void start_gen_wave(enum wave_type type) {
	HAL_TIM_Base_Start(&htim2);
	if (type == SINE_WAVE) {
		HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, sine_val, SAMPLES_NUM, DAC_ALIGN_12B_R);
	} else if (type == TRIANGLE_WAVE) {
		HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, triangle_val, SAMPLES_NUM, DAC_ALIGN_12B_R);
	}
}
