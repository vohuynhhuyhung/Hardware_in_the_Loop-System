/*
 * adc_fft.c
 *
 *  Created on: Apr 11, 2026
 *      Author: Samy Ve
 */

#include "adc_fft.h"
#include "usb.h"
#include "arm_math.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "stm32f4xx_hal.h"

#define ADC_BUFFER_SIZE 4096
#define SAMPLING_RATE   30000

extern ADC_HandleTypeDef hadc3;
extern TIM_HandleTypeDef htim8;

// DMA buffer
uint16_t adc_buffer[ADC_BUFFER_SIZE];

float32_t fft_input[ADC_BUFFER_SIZE];
float32_t fft_output_complex[ADC_BUFFER_SIZE];
float32_t fft_magnitude[ADC_BUFFER_SIZE / 2];

SemaphoreHandle_t fft_sem = NULL;
TaskHandle_t fftTaskHandle = NULL;

static uint32_t Process_FFT_And_Reply(void) {
	// Chuyển đổi và khử nhiễu DC
	for (int i = 0; i < ADC_BUFFER_SIZE; i++) {
		fft_input[i] = (float32_t)(adc_buffer[i] - 2048);
	}

	// Cấu hình và chạy RFFT
	arm_rfft_fast_instance_f32 rfft_inst;
	arm_rfft_fast_init_f32(&rfft_inst, ADC_BUFFER_SIZE);
	arm_rfft_fast_f32(&rfft_inst, fft_input, fft_output_complex, 0);

	// Tính biên độ
	arm_cmplx_mag_f32(fft_output_complex, fft_magnitude, ADC_BUFFER_SIZE / 2);
	fft_magnitude[0] = 0.0f; // Bỏ qua mốc 0Hz

	// Tìm đỉnh
	float32_t max_mag;
	uint32_t max_idx;
	arm_max_f32(&fft_magnitude[1], (ADC_BUFFER_SIZE / 2) - 1, &max_mag, &max_idx);
	max_idx += 1;

	// Quy đổi tần số
	uint32_t freq = (max_idx * SAMPLING_RATE) / ADC_BUFFER_SIZE;
	return freq;
}

static void FFT_Processing_Task(void *pvParameters) {
    while(1) {
        // Wait semaphore
        if (xSemaphoreTake(fft_sem, portMAX_DELAY) == pdTRUE) {
            uint32_t frequency = Process_FFT_And_Reply();
            Console_Write("Measured Frequency: %lu Hz\r\n", frequency);
        }
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    if(hadc->Instance == ADC3) {
        // Tắt DMA
        HAL_TIM_Base_Stop(&htim8);
        HAL_ADC_Stop_DMA(&hadc3);

        // Đánh thức Task đang ngủ bằng Semaphore
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(fft_sem, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void ADC_FFT_Init(void) {
    // Tạo Binary Semaphore
    fft_sem = xSemaphoreCreateBinary();
    xTaskCreate(FFT_Processing_Task, "FFT_Task", configMINIMAL_STACK_SIZE * 8, NULL, 3, &fftTaskHandle);
}

void ADC_FFT_TriggerCapture(void) {
    // Khởi động lại hệ thống lấy mẫu
    HAL_ADC_Start_DMA(&hadc3, (uint32_t*)adc_buffer, ADC_BUFFER_SIZE);
    HAL_TIM_Base_Start(&htim8);
}
