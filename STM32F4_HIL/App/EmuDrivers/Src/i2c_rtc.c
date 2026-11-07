/*
 * i2c_rtc.c
 * DS1307 emu
 *
 *  Created on: Nov 1, 2025
 *      Author: VyTran
 */

#include "i2c_rtc.h"
#include "i2c_device.h"
#include <string.h>

static t_rtc g_rtc;
extern TIM_HandleTypeDef htim3;
TIM_HandleTypeDef *rtc_htim = &htim3;

static uint8_t bcd2bin(uint8_t v)
{
    return ((v >> 4) * 10) + (v & 0x0F);
}

static uint8_t bin2bcd(uint8_t v)
{
    return ((v / 10) << 4) | (v % 10);
}

static bool is_leap_year(uint8_t year)
{
    return ((2000 + year) % 4) == 0;
}

static uint8_t days_in_month(uint8_t month, uint8_t year)
{
    static const uint8_t days[12] =
    {
        31,28,31,30,31,30,31,31,30,31,30,31
    };

    if(month == 2 && is_leap_year(year))
        return 29;

    return days[month - 1];
}

t_rtc* rtc_init(uint8_t addr)
{
    memset(&g_rtc, 0, sizeof(g_rtc));

    g_rtc.addr = addr;
    g_rtc.running = true;
    g_rtc.cur_reg = 0;
    g_rtc.rx_expect_reg = true;

    // Default time: 00:00:00 Thurday 01/01/26
    g_rtc.regs[0x00] = 0x00; // Seconds, CH=0
    g_rtc.regs[0x01] = 0x00; // Minutes
    g_rtc.regs[0x02] = 0x00; // Hours (24h)
    g_rtc.regs[0x03] = 0x05; // Day
    g_rtc.regs[0x04] = 0x01; // Date
    g_rtc.regs[0x05] = 0x01; // Month
    g_rtc.regs[0x06] = 0x26; // Year
    g_rtc.regs[0x07] = 0x00; // Control

    HAL_TIM_Base_Start_IT(rtc_htim);

    return &g_rtc;
}

void rtc_deinit(t_rtc *r)
{
	HAL_TIM_Base_Stop_IT(rtc_htim);
    memset(&g_rtc, 0, sizeof(g_rtc));
}

t_rtc* find_rtc(uint8_t addr)
{
    if (g_rtc.addr == addr && g_rtc.running)
        return &g_rtc;
    return NULL;
}

void rtc_set_time(t_rtc *r, rtc_time_t *t)
{
    r->regs[0] = bin2bcd(t->sec);
    r->regs[1] = bin2bcd(t->min);
    r->regs[2] = bin2bcd(t->hour);
}

void rtc_set_date(t_rtc *r, rtc_date_t *d)
{
    r->regs[3] = bin2bcd(d->day);
    r->regs[4] = bin2bcd(d->date);
    r->regs[5] = bin2bcd(d->month);
    r->regs[6] = bin2bcd(d->year);
}

void rtc_get_time(t_rtc *r, rtc_time_t *t)
{
    t->sec  = bcd2bin(r->regs[0]);
    t->min  = bcd2bin(r->regs[1]);
    t->hour = bcd2bin(r->regs[2]);
}

void rtc_get_date(t_rtc *r, rtc_date_t *d)
{
    d->day   = bcd2bin(r->regs[3]);
    d->date  = bcd2bin(r->regs[4]);
    d->month = bcd2bin(r->regs[5]);
    d->year  = bcd2bin(r->regs[6]);
}

void rtc_tick(void)
{
    if(!g_rtc.running)
        return;

    // CH bit = 1 => Halt Clock
    if(g_rtc.regs[0] & 0x80)
        return;

    uint8_t sec   = bcd2bin(g_rtc.regs[0] & 0x7F);
    uint8_t min   = bcd2bin(g_rtc.regs[1]);
    uint8_t hour  = bcd2bin(g_rtc.regs[2] & 0x3F);
    uint8_t day   = bcd2bin(g_rtc.regs[3]);
    uint8_t date  = bcd2bin(g_rtc.regs[4]);
    uint8_t month = bcd2bin(g_rtc.regs[5]);
    uint8_t year  = bcd2bin(g_rtc.regs[6]);

    if(++sec >= 60)
    {
        sec = 0;

        if(++min >= 60)
        {
            min = 0;

            if(++hour >= 24)
            {
                hour = 0;

                day++;
                if(day > 7) day = 1;

                date++;
                if(date > days_in_month(month, year))
                {
                    date = 1;
                    month++;

                    if(month > 12)
                    {
                        month = 1;
                        year++;
                    }
                }
            }
        }
    }

    g_rtc.regs[0] = (g_rtc.regs[0] & 0x80) | bin2bcd(sec);
    g_rtc.regs[1] = bin2bcd(min);
    g_rtc.regs[2] = bin2bcd(hour);   // 24h only
    g_rtc.regs[3] = bin2bcd(day);
    g_rtc.regs[4] = bin2bcd(date);
    g_rtc.regs[5] = bin2bcd(month);
    g_rtc.regs[6] = bin2bcd(year);
}

void rtc_addr_handler(I2C_HandleTypeDef *hi2c, uint8_t direction, uint8_t addr)
{
    if(direction == I2C_DIRECTION_TRANSMIT)
    {
        g_rtc.rx_expect_reg = true;

        HAL_I2C_Slave_Sequential_Receive_IT(
            hi2c,
            &g_rtc.rx_byte,
            1,
            I2C_FIRST_FRAME);
    }
    else
    {
        g_rtc.tx_byte = g_rtc.regs[g_rtc.cur_reg];

        HAL_I2C_Slave_Sequential_Transmit_IT(
            hi2c,
            &g_rtc.tx_byte,
            1,
            I2C_FIRST_FRAME);
    }
}

void rtc_rx_handler(I2C_HandleTypeDef *hi2c)
{
    uint8_t rx = g_rtc.rx_byte;

    if(g_rtc.rx_expect_reg)
    {
        g_rtc.cur_reg = rx & 0x3F;
        g_rtc.rx_expect_reg = false;
    }
    else
    {
        g_rtc.regs[g_rtc.cur_reg] = rx;
        g_rtc.cur_reg = (g_rtc.cur_reg + 1) & 0x3F;
    }

    HAL_I2C_Slave_Sequential_Receive_IT(
        hi2c,
        &g_rtc.rx_byte,
        1,
        I2C_NEXT_FRAME);
}

void rtc_tx_handler(I2C_HandleTypeDef *hi2c)
{
    g_rtc.cur_reg = (g_rtc.cur_reg + 1) & 0x3F;

    g_rtc.tx_byte = g_rtc.regs[g_rtc.cur_reg];

    HAL_I2C_Slave_Sequential_Transmit_IT(
        hi2c,
        &g_rtc.tx_byte,
        1,
        I2C_NEXT_FRAME);
}

void rtc_listen_handler(I2C_HandleTypeDef *hi2c)
{
    g_rtc.rx_expect_reg = true;
}

