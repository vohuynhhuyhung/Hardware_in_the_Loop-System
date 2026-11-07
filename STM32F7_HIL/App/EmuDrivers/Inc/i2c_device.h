/*
* i2c_device.h
*
* Created on: Oct 10, 2025
* Author: VyTran
*
*/
#ifndef I2C_DEVICE_H
#define I2C_DEVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f7xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#define I2C_DEV_MAX 127

typedef enum {
    I2C_STATE_UNUSED = 0,
    I2C_STATE_INITED,
    I2C_STATE_USING
} i2c_dev_state_t;

typedef enum {
    I2C_DEV_UNKNOWN = 0,
    I2C_DEV_EEPROM,
    I2C_DEV_RTC
} i2c_dev_type_t;

typedef struct {
    i2c_dev_type_t type;
    i2c_dev_state_t state;
} i2c_dev_t;

extern i2c_dev_t i2c_dev_table[I2C_DEV_MAX];

void i2c_emu_device_init(void);
void i2c_emu_add_device(i2c_dev_type_t dev_type, uint8_t addr7);
int  i2c_set_slave_addr(uint8_t addr7);
void i2c_emu_remove_device(uint8_t addr7);


#endif // I2C_DEVICE_H
