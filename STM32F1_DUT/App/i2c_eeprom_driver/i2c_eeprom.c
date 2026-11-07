/*
 * i2c_eeprom.c
 *
 *  Created on: Dec 9, 2025
 *      Author: Vy Tran
 */

#include "i2c_eeprom.h"
static HAL_StatusTypeDef EEPROM_WaitReady(I2C_EEPROM_t *eeprom)
{
    return HAL_I2C_IsDeviceReady(eeprom->hi2c,
                                 eeprom->dev_addr,
                                 5,
                                 10);
}

HAL_StatusTypeDef EEPROM_Init(I2C_EEPROM_t *eeprom,
                              I2C_HandleTypeDef *hi2c,
                              uint8_t dev7bit,
                              uint16_t mem_size,
                              uint16_t page_size,
                              uint8_t addr_size)
{
    if (!eeprom || !hi2c)
        return HAL_ERROR;

    eeprom->hi2c      = hi2c;
    eeprom->dev_addr  = dev7bit << 1;
    eeprom->mem_size  = mem_size;
    eeprom->page_size = page_size;
    eeprom->addr_size = addr_size;

    return EEPROM_WaitReady(eeprom);
}

/* WRITE ARRAY (PAGE ALIGNED) */
HAL_StatusTypeDef EEPROM_Write(I2C_EEPROM_t *eeprom,
                               uint16_t mem_addr,
                               uint8_t *data,
                               uint16_t len)
{
    if (!eeprom || !data)
        return HAL_ERROR;

    if ((mem_addr + len) > eeprom->mem_size)
        return HAL_ERROR;

    uint16_t offset = 0;

    while (offset < len) {
        uint16_t page_offset = mem_addr % eeprom->page_size;
        uint16_t chunk = eeprom->page_size - page_offset;

        if (chunk > (len - offset))
            chunk = len - offset;

        if (HAL_I2C_Mem_Write(eeprom->hi2c,
                              eeprom->dev_addr,
                              mem_addr,
                              eeprom->addr_size,
                              &data[offset],
                              chunk,
                              1000) != HAL_OK)
        {
            return HAL_ERROR;
        }

        if (EEPROM_WaitReady(eeprom) != HAL_OK)
            return HAL_ERROR;

        mem_addr += chunk;
        offset   += chunk;
    }

    return HAL_OK;
}

/* READ ARRAY */
HAL_StatusTypeDef EEPROM_Read(I2C_EEPROM_t *eeprom,
                              uint16_t mem_addr,
                              uint8_t *data,
                              uint16_t len)
{
    if (!eeprom || !data)
        return HAL_ERROR;

    if ((mem_addr + len) > eeprom->mem_size)
        return HAL_ERROR;

    return HAL_I2C_Mem_Read(eeprom->hi2c,
                            eeprom->dev_addr,
                            mem_addr,
                            eeprom->addr_size,
                            data,
                            len,
                            1000);
}

/* AUTO FILL TEST DATA: 0,1,2,3... */
HAL_StatusTypeDef EEPROM_Fill(I2C_EEPROM_t *eeprom,
                              uint16_t mem_addr,
                              uint16_t len)
{
    uint8_t buf[32];

    for (uint16_t i = 0; i < len; i += sizeof(buf)) {
        uint16_t chunk = len - i;
        if (chunk > sizeof(buf))
            chunk = sizeof(buf);

        for (uint16_t j = 0; j < chunk; j++)
            buf[j] = (uint8_t)(i + j);

        if (EEPROM_Write(eeprom,
                         mem_addr + i,
                         buf,
                         chunk) != HAL_OK)
            return HAL_ERROR;
    }

    return HAL_OK;
}
