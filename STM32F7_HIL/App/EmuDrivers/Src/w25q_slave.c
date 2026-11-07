/*
 * w25q_slave.c
 *
 *  Created on: Sep 27, 2025
 *      Author: Samy Ve
 */

#include "w25q_slave.h"

static uint8_t dummy = 0xFF;

W25Q_Slave w25q = {
	.spi = SPI5,
	.device_id = {0xEF, 0x40, 0x18}, // ID cho W25Q128
	.address = 0,
	.cmd = W25Q_NO_CMD,
	.rx_count = 0,
	.tx_count = 0,
	.rx_data = 0,
	.is_cmd_received = false,
	.count_byte_address = 0
};

// Hàm khởi tạo W25Q_Slave
void W25Q_Slave_Init(W25Q_Slave* dev) {
	memset(dev->memory, 0xFF, sizeof(dev->memory)); // Khởi tạo bộ nhớ mô phỏng với giá trị 0xFF
	LL_SPI_TransmitData8(w25q.spi, dummy);

	// Cấu hình EXTI cho PF6 (NSS)
	LL_EXTI_InitTypeDef EXTI_InitStruct = {0};
	LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTF, LL_SYSCFG_EXTI_LINE6);
	EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_6;
	EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
	EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_FALLING;
	EXTI_InitStruct.LineCommand = ENABLE;
	LL_EXTI_Init(&EXTI_InitStruct);
	NVIC_SetPriority(EXTI9_5_IRQn, 6);
	NVIC_EnableIRQ(EXTI9_5_IRQn);
}

static void Reset_W25Q (W25Q_Slave* dev) {
	dev->cmd = W25Q_NO_CMD;
	dev->rx_count = 0;
	dev->tx_count = 0;
	dev->address = 0;
	dev->is_cmd_received = false;
	dev->count_byte_address = 0;
}

void W25Q_Slave_IRQHandler(W25Q_Slave *dev, uint8_t rx_data) {
	if (dev->is_cmd_received == false) {
		dev->cmd = rx_data;
		dev->is_cmd_received = true;
		switch (dev->cmd) {
		case W25Q_READ_ID_CMD:
			LL_SPI_TransmitData8(dev->spi, dev->device_id[dev->tx_count++]);
			break;

		case W25Q_READ_STATUS_REG1_CMD:
			LL_SPI_TransmitData8(dev->spi, 0x00); // Vì master đợi status = 0x00
			break;

		case W25Q_CHIP_ERASE_CMD:
			memset(dev->memory, 0xFF, sizeof(dev->memory));
			LL_SPI_TransmitData8(dev->spi, 0x00);
			break;

		default:
			// Các lệnh khác cần thêm thời gian dummy
			LL_SPI_TransmitData8(dev->spi, dummy);
			break;
		}
	} else {
		switch (dev->cmd) {
		case W25Q_READ_ID_CMD:
			if (dev->tx_count < 3) {
				LL_SPI_TransmitData8(dev->spi, dev->device_id[dev->tx_count++]);
			}
			break;
		case W25Q_READ_CMD:
			if (dev->count_byte_address < 3) {
				dev->address = (dev->address << 8) | rx_data;
				dev->count_byte_address++;
				dev->address &= 0x00FFFFFF;
				// Sau khi tăng count, kiểm tra nếu address hoàn chỉnh (count=3)
				if (dev->count_byte_address == 3) {
					LL_SPI_TransmitData8(dev->spi, dev->memory[dev->address + dev->tx_count++]); // Gửi memory[address] cho chu kỳ tiếp
				} else {
					LL_SPI_TransmitData8(dev->spi, dummy);
				}
			} else {
				if ((dev->address + dev->tx_count) < W25Q_MEMORY_SIZE-1) {
					LL_SPI_TransmitData8(dev->spi, dev->memory[dev->address + dev->tx_count++]);
				} else {
					LL_SPI_TransmitData8(dev->spi, dummy);
				}
			}
			break;
		case W25Q_WRITE_ENABLE_CMD:
			dev->is_cmd_received = false;
			break;
		case W25Q_PAGE_PROGRAM_CMD:
			if (dev->count_byte_address < 3) {
				dev->address = (dev->address << 8) | rx_data;
				dev->count_byte_address++;
				dev->address &= 0x00FFFFFF;
			} else {
				dev->memory[dev->address + dev->rx_count++] = rx_data;
			}
			break;
		default:
			break;
		}
	}
}

void W25Q_PrepareData(W25Q_Slave *dev, uint32_t length)
{
	if (length > W25Q_MEMORY_SIZE) {
		length = W25Q_MEMORY_SIZE; // tránh tràn
	}
	memset(dev->memory, 0xFF, sizeof(dev->memory)); // data default trong memory
	for (uint32_t i = 0; i < length; i++) {
		dev->memory[i] = (uint8_t)(i & 0xFF); // dữ liệu tăng dần từ 0..255
	}
}

void EXTI9_5_IRQHandler(void)
{
	if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_6)) {
		LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_6);
		if (LL_GPIO_IsInputPinSet(GPIOF, LL_GPIO_PIN_6) == 0) {
			// NSS falling → bắt đầu transaction mới
			Reset_W25Q(&w25q);
		}
	}
}
