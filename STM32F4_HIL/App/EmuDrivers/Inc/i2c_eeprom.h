/*
* i2c_eeprom.h
* Created on: Sep 19, 2025
* Author: VyTran
*/

#ifndef I2C_EEPROM_H
#define I2C_EEPROM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_EEPROM 4

typedef struct {
    I2C_HandleTypeDef *i2c;
    uint8_t  *mem;
    uint16_t page_size;
    uint16_t eeprom_size;
    uint8_t  eeprom_addr;     // 7-bit
    uint16_t current_addr;

    uint8_t  *page_buf;
    uint16_t page_write_addr;
    uint16_t page_offset;
    bool     writing_page;
    bool     got_addr;
} t_eeprom;

t_eeprom* eeprom_init(I2C_HandleTypeDef *hi2c, uint8_t addr7, uint16_t size, uint16_t page_size);
void eeprom_deinit(t_eeprom *e);
t_eeprom* find_eeprom(uint16_t addr7);

// Callbacks
void eeprom_addr_handler(I2C_HandleTypeDef *hi2c, uint8_t direction, uint8_t addr);
void eeprom_rx_handler(I2C_HandleTypeDef *hi2c);
void eeprom_tx_handler(I2C_HandleTypeDef *hi2c);
void eeprom_listen_handler(I2C_HandleTypeDef *hi2c);

#ifdef __cplusplus
}
#endif

#endif /*I2C_EEPROM_H*/
