/*
 * i2c_eeprom.c
 *
 *  Created on: Dec 9, 2025
 *      Author: Vy Tran
 */
#ifndef I2C_EEPROM_H
#define I2C_EEPROM_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t  dev_addr;     // 7-bit << 1
    uint16_t mem_size;     // total size (bytes)
    uint16_t page_size;    // page size (bytes)
    uint8_t  addr_size;    // I2C_MEMADD_SIZE_8BIT or 16BIT
} I2C_EEPROM_t;

/* API */
/* Init */
HAL_StatusTypeDef EEPROM_Init(I2C_EEPROM_t *eeprom,
                              I2C_HandleTypeDef *hi2c,
                              uint8_t dev7bit,
                              uint16_t mem_size,
                              uint16_t page_size,
                              uint8_t addr_size);

/* Basic read / write */
HAL_StatusTypeDef EEPROM_Write(I2C_EEPROM_t *eeprom,
                               uint16_t mem_addr,
                               uint8_t *data,
                               uint16_t len);

HAL_StatusTypeDef EEPROM_Read(I2C_EEPROM_t *eeprom,
                              uint16_t mem_addr,
                              uint8_t *data,
                              uint16_t len);

/* Test helper */
HAL_StatusTypeDef EEPROM_Fill(I2C_EEPROM_t *eeprom,
                              uint16_t mem_addr,
                              uint16_t len);

#endif

