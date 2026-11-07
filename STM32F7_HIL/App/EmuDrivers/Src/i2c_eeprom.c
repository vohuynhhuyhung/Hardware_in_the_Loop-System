/*
* i2c_eeprom.c
* Created on: Sep 19, 2025
* Author: VyTran
*/

#include <stdlib.h>
#include <string.h>
#include "i2c_eeprom.h"
#include "i2c_device.h"

static t_eeprom ee_list[MAX_EEPROM];
static uint8_t ee_count = 0;

static t_eeprom *active_eeprom = NULL;
static uint8_t rx_buf[2];   // buffer nhận địa chỉ

t_eeprom* find_eeprom(uint16_t addr7)
{
    for (uint8_t i = 0; i < ee_count; i++) {
        if (addr7 == ee_list[i].eeprom_addr)
            return &ee_list[i];
    }
    return NULL;
}

t_eeprom* eeprom_init(I2C_HandleTypeDef *hi2c,
                      uint8_t addr7,
                      uint16_t size,
                      uint16_t page_size)
{
    if (ee_count >= MAX_EEPROM) return NULL;
    if (page_size > size) return NULL;

    t_eeprom *e = &ee_list[ee_count];

    e->i2c         = hi2c;
    e->eeprom_addr = addr7;
    e->eeprom_size = size;
    e->page_size   = page_size;

    e->mem      = malloc(size);
    e->page_buf = malloc(page_size);

    if (!e->mem || !e->page_buf) {
        free(e->mem);
        free(e->page_buf);
        return NULL;
    }

    memset(e->mem, 0xFF, size);

    e->current_addr   = 0;
    e->page_offset    = 0;
    e->writing_page   = false;
    e->got_addr       = false;

    ee_count++;
    return e;
}

void eeprom_deinit(t_eeprom *e)
{
    if (!e) return;

    free(e->mem);
    free(e->page_buf);

    uint8_t idx = e - ee_list;

    if (idx < ee_count - 1) {
        memmove(&ee_list[idx], &ee_list[idx + 1],
                sizeof(t_eeprom) * (ee_count - idx - 1));
    }

    ee_count--;

    if (active_eeprom == e)
        active_eeprom = NULL;
}

void eeprom_addr_handler(I2C_HandleTypeDef *hi2c,
                         uint8_t direction,
                         uint8_t addr7)
{
    t_eeprom *e = find_eeprom(addr7);
    if (!e) return;

    active_eeprom = e;

    if (direction == I2C_DIRECTION_TRANSMIT) {

        e->got_addr = false;
        HAL_I2C_Slave_Sequential_Receive_IT(hi2c, rx_buf, 2, I2C_FIRST_FRAME);
    }
    else {
        HAL_I2C_Slave_Sequential_Transmit_IT(
            hi2c, &e->mem[e->current_addr], 1, I2C_FIRST_FRAME);
    }
}

void eeprom_rx_handler(I2C_HandleTypeDef *hi2c)
{
    if (!active_eeprom) return;
    t_eeprom *e = active_eeprom;

    if (!e->got_addr) {

        e->got_addr = true;

        e->current_addr =
            (((uint16_t)rx_buf[0] << 8) | rx_buf[1]) % e->eeprom_size;

        e->page_write_addr = e->current_addr;
        e->page_offset     = 0;
        e->writing_page    = true;

        HAL_I2C_Slave_Sequential_Receive_IT(hi2c, rx_buf, 1, I2C_NEXT_FRAME);
    }
    else {

        if (e->page_offset < e->page_size)
            e->page_buf[e->page_offset++] = rx_buf[0];

        e->current_addr = (e->current_addr + 1) % e->eeprom_size;

        HAL_I2C_Slave_Sequential_Receive_IT(hi2c, rx_buf, 1, I2C_NEXT_FRAME);
    }
}

void eeprom_tx_handler(I2C_HandleTypeDef *hi2c)
{
    if (!active_eeprom) return;

    t_eeprom *e = active_eeprom;

    e->current_addr = (e->current_addr + 1) % e->eeprom_size;

    HAL_I2C_Slave_Sequential_Transmit_IT(
        hi2c, &e->mem[e->current_addr], 1, I2C_NEXT_FRAME);
}

void eeprom_listen_handler(I2C_HandleTypeDef *hi2c)
{
    if (active_eeprom && active_eeprom->writing_page) {

        t_eeprom *e = active_eeprom;
        uint16_t a  = e->page_write_addr;

        for (uint16_t i = 0; i < e->page_offset; i++) {
            e->mem[a % e->eeprom_size] = e->page_buf[i];
            a++;
        }

        e->writing_page = false;
    }

    if (active_eeprom)
        active_eeprom->got_addr = false;

    active_eeprom = NULL;

    HAL_I2C_EnableListen_IT(hi2c);
}

