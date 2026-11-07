/*
 * can_driver.c
 *
 *  Created on: Feb 16, 2026
 *      Author: Samy Ve
 */

#include "can_driver.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

extern CAN_HandleTypeDef hcan1;

struct can_driver_t can_driver = {
	.tx_header = {0},
	.rx_header = {0},
	.tx_mailbox = 0,
	.tx_data = {0},
	.rx_data = {0},
	.rx_buffer = {0}
};

void CAN_Init(void) {
	can_driver.hcan = hcan1;
	// Cấu hình CAN Tx Header
	can_driver.tx_header.StdId = 0x123; // ID chuẩn
	can_driver.tx_header.RTR = CAN_RTR_DATA; // Loại frame: dữ liệu
	can_driver.tx_header.IDE = CAN_ID_STD; // Sử dụng ID chuẩn

	CAN_FilterTypeDef canfilterconfig;
	canfilterconfig.FilterActivation = CAN_FILTER_ENABLE;
	canfilterconfig.FilterBank = 18;  // which filter bank to use from the assigned ones
	canfilterconfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
	canfilterconfig.FilterIdHigh = 0x407<<5;
	canfilterconfig.FilterIdLow = 0;
	canfilterconfig.FilterMaskIdHigh = 0x407<<5;
	canfilterconfig.FilterMaskIdLow = 0x0000;
	canfilterconfig.FilterMode = CAN_FILTERMODE_IDMASK;
	canfilterconfig.FilterScale = CAN_FILTERSCALE_32BIT;
	canfilterconfig.SlaveStartFilterBank = 20;  // how many filters to assign to the CAN1 (master can)
	HAL_CAN_ConfigFilter(&hcan1, &canfilterconfig);

	HAL_CAN_Start(&can_driver.hcan);
	HAL_CAN_ActivateNotification(&can_driver.hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
}

void CAN_Send(uint8_t *data, uint8_t length) {
	if (length > 8) {
		length = 8; // CAN chỉ hỗ trợ tối đa 8 byte dữ liệu
	}
	memcpy(can_driver.tx_data, data, length);
	can_driver.tx_header.DLC = length;
	HAL_CAN_AddTxMessage(&can_driver.hcan, &can_driver.tx_header, can_driver.tx_data, &can_driver.tx_mailbox);
}

void CAN_SendBuffer(void) {
	uint8_t buffer[256];
	for (uint16_t i = 0; i < 256; i++) {
		buffer[i] = i; // tạo dữ liệu 0x00–0xFF
	}

	// gửi từng khung 8 byte
	for (uint16_t offset = 0; offset < 256; offset += 8) {
		CAN_Send(&buffer[offset], 8);
		vTaskDelay(pdMS_TO_TICKS(10)); // delay nhỏ để tránh quá tải bus CAN
	}
}

void CAN_Send_Byte(uint8_t data) {
	// Cấu hình DLC = 1 byte
	can_driver.tx_header.DLC = 1;
	// Copy dữ liệu vào buffer truyền
	can_driver.tx_data[0] = data;
	// Gửi message
	HAL_CAN_AddTxMessage(&can_driver.hcan, &can_driver.tx_header, can_driver.tx_data, &can_driver.tx_mailbox);
}

void CAN_Send_String(const char *str) {
	uint16_t len = strlen(str);

	for (uint16_t i = 0; i < len; i++) {
		CAN_Send_Byte((uint8_t)str[i]);

		// Lưu ý: CAN có tốc độ cao nhưng nếu gửi liên tục 1 byte/frame
		// có thể gây nghẽn bus nếu nhiều thiết bị cùng gửi.
		// Bạn có thể thêm một khoảng delay cực nhỏ nếu cần thiết.
		vTaskDelay(1); // Nếu dùng FreeRTOS
	}
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
	HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &can_driver.rx_header, can_driver.rx_data);
	// copy dữ liệu nhận vào rx_buffer
	for (int i = 0; i < can_driver.rx_header.DLC; i++) {
		if (can_driver.rx_index < sizeof(can_driver.rx_buffer)) {
			can_driver.rx_buffer[can_driver.rx_index++] = can_driver.rx_data[i];
		} else {
			// buffer overflow, reset index
			can_driver.rx_index = 0;
			can_driver.rx_buffer[can_driver.rx_index++] = can_driver.rx_data[i];
		}
	}
}
