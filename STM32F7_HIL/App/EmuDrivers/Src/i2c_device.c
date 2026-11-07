/*
 * i2c_device.c
 *
 *  Created on: Oct 10, 2025
 *      Author: VyTran
 */
#include "i2c_device.h"
#include "i2c_eeprom.h"
#include "i2c_rtc.h"

extern I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef *hi2c = &hi2c1;

i2c_dev_t i2c_dev_table[I2C_DEV_MAX];

static uint8_t active_addr = 0xFF;

void i2c_emu_device_init(void)
{
    for (int i = 0; i <= I2C_DEV_MAX; i++)
    {
        i2c_dev_table[i].type  = I2C_DEV_UNKNOWN;
        i2c_dev_table[i].state = I2C_STATE_UNUSED;
    }
}

void i2c_emu_add_device(i2c_dev_type_t dev_type, uint8_t addr7)
{
    if (addr7 > I2C_DEV_MAX) return;

    i2c_dev_table[addr7].type  = dev_type;
    i2c_dev_table[addr7].state = I2C_STATE_INITED;
}

void i2c_emu_remove_device(uint8_t addr7)
{
    if (addr7 > I2C_DEV_MAX) return;

    i2c_dev_table[addr7].type  = I2C_DEV_UNKNOWN;
    i2c_dev_table[addr7].state = I2C_STATE_UNUSED;
}

int i2c_set_slave_addr(uint8_t addr7)
{
    if (addr7 > I2C_DEV_MAX) return -1;

    if (i2c_dev_table[addr7].state != I2C_STATE_INITED) return -2;

    for (int i = 0; i <= I2C_DEV_MAX; i++)
    {
    	if (i2c_dev_table[i].state == I2C_STATE_USING){
            i2c_dev_table[i].state = I2C_STATE_INITED;
    	}
    }

    i2c_dev_table[addr7].state = I2C_STATE_USING;

    HAL_I2C_DeInit(hi2c);
    hi2c->Init.OwnAddress1 = (addr7 << 1);
    HAL_I2C_Init(hi2c);

    HAL_I2C_EnableListen_IT(hi2c);
    active_addr = addr7;

    return 0;
}

// Callback
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection,
                          uint16_t AddrMatchCode)
{
    uint8_t addr7 = AddrMatchCode >> 1;
    active_addr = addr7;

    switch (i2c_dev_table[addr7].type)
    {
        case I2C_DEV_EEPROM:
            eeprom_addr_handler(hi2c, TransferDirection, addr7);
            break;

        case I2C_DEV_RTC:
            rtc_addr_handler(hi2c, TransferDirection, addr7);
            break;

        default:
            break;
    }
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (active_addr == 0xFF) return;

    switch (i2c_dev_table[active_addr].type)
    {
        case I2C_DEV_EEPROM:
            eeprom_rx_handler(hi2c);
            break;

        case I2C_DEV_RTC:
            rtc_rx_handler(hi2c);
            break;

        default:
            break;
    }
}

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (active_addr == 0xFF) return;

    switch (i2c_dev_table[active_addr].type)
    {
        case I2C_DEV_EEPROM:
            eeprom_tx_handler(hi2c);
            break;

        case I2C_DEV_RTC:
            rtc_tx_handler(hi2c);
            break;

        default:
            break;
    }
}

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (active_addr != 0xFF)
    {
        switch (i2c_dev_table[active_addr].type)
        {
            case I2C_DEV_EEPROM:
                eeprom_listen_handler(hi2c);
                break;

            case I2C_DEV_RTC:
                rtc_listen_handler(hi2c);
                break;

            default:
                break;
        }
    }

    active_addr = 0xFF;
    HAL_I2C_EnableListen_IT(hi2c);
}
