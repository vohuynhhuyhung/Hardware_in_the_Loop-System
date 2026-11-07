/*
 * can_driver.h
 *
 *  Created on: Feb 16, 2026
 *      Author: Samy Ve
 */

#ifndef CAN_DRIVER_CAN_DRIVER_H_
#define CAN_DRIVER_CAN_DRIVER_H_

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_can.h"
#include <stdint.h>
#include <stdbool.h>

struct can_driver_t {
	CAN_HandleTypeDef hcan;
	CAN_TxHeaderTypeDef tx_header;
	CAN_RxHeaderTypeDef rx_header;
	uint32_t tx_mailbox;
	uint8_t tx_data[8];
	uint8_t rx_data[8];
	uint16_t rx_index;
	uint8_t rx_buffer[256];
};

extern struct can_driver_t can_driver;

void CAN_Init(void);
void CAN_Send(uint8_t *data, uint8_t length);
void CAN_SendBuffer(void);

#endif /* CAN_DRIVER_CAN_DRIVER_H_ */
