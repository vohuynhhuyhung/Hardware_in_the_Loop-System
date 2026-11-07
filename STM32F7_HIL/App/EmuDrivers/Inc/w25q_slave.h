/*
 * w25q_slave.h
 *
 *  Created on: Sep 27, 2025
 *      Author: Samy Ve
 */

#ifndef W25Q_SLAVE_W25Q_SLAVE_H_
#define W25Q_SLAVE_W25Q_SLAVE_H_

#include "stm32f7xx_ll_spi.h" 
#include "stm32f7xx_ll_dma.h"
#include "stm32f7xx_ll_system.h"
#include "stm32f7xx_ll_exti.h"
#include "stm32f7xx_ll_gpio.h"
#include "stm32f7xx_ll_exti.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdbool.h>

// Định nghĩa các lệnh SPI cho W25Q
#define W25Q_NO_CMD  0x00
#define W25Q_READ_CMD  0x03
#define W25Q_READ_ID_CMD 0x9F
#define W25Q_WRITE_ENABLE_CMD 0x06
#define W25Q_CHIP_ERASE_CMD   0xC7
#define W25Q_READ_STATUS_REG1_CMD 0x05
#define W25Q_PAGE_PROGRAM_CMD 0x02

#define W25Q_PAGE_SIZE 256
#define W25Q_MEMORY_SIZE 1024

typedef struct {
    SPI_TypeDef* spi;           // Con trỏ tới SPI peripheral
    uint8_t device_id[3];      // ID thiết bị
    uint32_t address;                         // Địa chỉ hiện tại
    uint8_t cmd;                              // Lệnh nhận được
    uint16_t rx_count;                        // Số byte đã nhận
    uint16_t tx_count;                        // Số byte đã gửi
    uint8_t rx_data;    // Buffer nhận dữ liệu (lệnh + địa chỉ + dữ liệu)
    uint8_t memory[W25Q_MEMORY_SIZE];         // Bộ nhớ mô phỏng cho W25Q32JV
    bool is_cmd_received;                     // Cờ lệnh đã nhận
    uint8_t count_byte_address;
} W25Q_Slave;

extern W25Q_Slave w25q;


void W25Q_Slave_Init(W25Q_Slave* dev);
void W25Q_Slave_IRQHandler(W25Q_Slave* dev, uint8_t rx_data);
void W25Q_PrepareData(W25Q_Slave *dev, uint32_t length);

#endif /* W25Q_SLAVE_W25Q_SLAVE_H_ */
