/*
 * w25q_driver.h
 *
 *  Created on: Sep 23, 2025
 *      Author: Samy Ve
 */

#ifndef W25Q_DRIVER_W25Q_DRIVER_H_
#define W25Q_DRIVER_W25Q_DRIVER_H_

#include "main.h"
#include <stdbool.h>
#include "stm32f1xx_ll_spi.h"
#include "stm32f1xx_ll_gpio.h"

// Định nghĩa lệnh
#define W25Q_PAGE_SIZE 256
#define W25Q_READ_CMD  0x03
#define W25Q_WRITE_CMD 0x02
#define W25Q_READ_ID_CMD 0x9F
#define W25Q_FAST_READ_CMD 0x0B
#define W25Q_WRITE_ENABLE_CMD 0x06
#define W25Q_CHIP_ERASE_CMD   0xC7
#define W25Q_READ_STATUS_REG1_CMD 0x05
#define W25Q_PAGE_PROGRAM_CMD 0x02

// Struct cấu hình W25Q
typedef struct {
    SPI_TypeDef *spi;               // SPI instance
    GPIO_TypeDef *cs_port;          // CS Port
    uint32_t cs_pin;                // CS pin
} W25Q_t;

extern W25Q_t W25Q;

void W25Q_Init(W25Q_t *config);

void W25Q_read_id(W25Q_t *config, uint8_t *buffer);

void W25Q_page_program(W25Q_t *config, uint32_t address,
                       uint32_t size, const uint8_t *buffer);

void W25Q_write(W25Q_t *config, uint32_t address,
                uint32_t size, const uint8_t *buffer);

void W25Q_read(W25Q_t *config, uint32_t address,
               uint32_t size, uint8_t *buffer);

void W25Q_fast_read(W25Q_t *config, uint32_t address,
                    uint32_t size, uint8_t *buffer);

void W25Q_chip_erase(W25Q_t *config);

#endif /* W25Q_DRIVER_W25Q_DRIVER_H_ */
