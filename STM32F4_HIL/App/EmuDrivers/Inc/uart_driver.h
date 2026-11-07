/*
 * uart_driver.h
 *
 *  Created on: Feb 18, 2026
 *      Author: VyTran
 */

#ifndef EMUDRIVERS_INC_UART_DRIVER_H_
#define EMUDRIVERS_INC_UART_DRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#define UART_RX_BUF_LEN 512

void UART_Init(void);
uint16_t UART_Available(void);
void UART_Clear(void);
uint16_t UART_Read(uint8_t *buf, uint16_t len);
void UART_Send(uint8_t *data, uint16_t len);
void UART_Send_String(const char *str);

#endif /* EMUDRIVERS_INC_UART_DRIVER_H_ */
