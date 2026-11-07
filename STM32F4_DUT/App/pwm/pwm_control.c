/*
 * adc.c
 *
 *  Created on: Mar 10, 2026
 *      Author: VyTran
 */

#include "pwm_control.h"

extern TIM_HandleTypeDef htim3;
static TIM_HandleTypeDef *pwm_tim = &htim3;

#define VDC_MV 3300
#define TIM_CLOCK 60000000
#define ARR_MAX 0xFFFFULL

int pwm_start(uint8_t ch)
{
	switch (ch) {
		case 1:
		    HAL_TIM_PWM_Start(pwm_tim, TIM_CHANNEL_1);
			break;
		case 2:
		    HAL_TIM_PWM_Start(pwm_tim, TIM_CHANNEL_2);
			break;
		case 3:
		    HAL_TIM_PWM_Start(pwm_tim, TIM_CHANNEL_3);
			break;
		case 4:
		    HAL_TIM_PWM_Start(pwm_tim, TIM_CHANNEL_4);
			break;
		default:
			return 1;
	}
	return 0;
}

int pwm_stop(uint8_t ch)
{
	switch (ch) {
		case 1:
		    HAL_TIM_PWM_Stop(pwm_tim, TIM_CHANNEL_1);
			break;
		case 2:
			HAL_TIM_PWM_Stop(pwm_tim, TIM_CHANNEL_2);
			break;
		case 3:
			HAL_TIM_PWM_Stop(pwm_tim, TIM_CHANNEL_3);
			break;
		case 4:
			HAL_TIM_PWM_Stop(pwm_tim, TIM_CHANNEL_4);
			break;
		default:
			return 1;
	}
	return 0;
}

void set_pwm_freq(uint64_t freq_hz)
{
    uint64_t psc = 0;
    uint64_t arr;

    arr = TIM_CLOCK / freq_hz;

    while(arr > ARR_MAX)
    {
        psc++;
        arr = TIM_CLOCK / ((psc + 1) * freq_hz);
    }

    __HAL_TIM_SET_PRESCALER(pwm_tim, psc);
    __HAL_TIM_SET_AUTORELOAD(pwm_tim, arr - 1);
    __HAL_TIM_SET_COUNTER(pwm_tim, 0);
}

void set_pwm_voltage(uint8_t ch, uint16_t voltage_mv)
{
	if(voltage_mv > VDC_MV) voltage_mv = VDC_MV;
    float duty = (float)voltage_mv / VDC_MV;

    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(pwm_tim);
    uint32_t ccr = duty * arr;

    switch(ch)
    {
        case 1:
            __HAL_TIM_SET_COMPARE(pwm_tim, TIM_CHANNEL_1, ccr);
            break;
        case 2:
            __HAL_TIM_SET_COMPARE(pwm_tim, TIM_CHANNEL_2, ccr);
            break;
        case 3:
            __HAL_TIM_SET_COMPARE(pwm_tim, TIM_CHANNEL_3, ccr);
            break;
        case 4:
            __HAL_TIM_SET_COMPARE(pwm_tim, TIM_CHANNEL_4, ccr);
            break;
    }
}

void set_pwm_duty_cycle(uint8_t ch, uint8_t duty)
{
    if(duty > 100) duty = 100;

    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(pwm_tim);
    uint32_t ccr = ((uint32_t)duty * (arr + 1)) / 100;

    switch(ch)
    {
      case 1:
            __HAL_TIM_SET_COMPARE(pwm_tim, TIM_CHANNEL_1, ccr);
            break;
        case 2:
            __HAL_TIM_SET_COMPARE(pwm_tim, TIM_CHANNEL_2, ccr);
            break;
        case 3:
            __HAL_TIM_SET_COMPARE(pwm_tim, TIM_CHANNEL_3, ccr);
            break;
        case 4:
            __HAL_TIM_SET_COMPARE(pwm_tim, TIM_CHANNEL_4, ccr);
            break;
    }
}
