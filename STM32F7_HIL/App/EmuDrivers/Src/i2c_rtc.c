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

static uint8_t bcd2bin(uint8_t v){ return (v>>4)*10 + (v&0x0F); }
static uint8_t bin2bcd(uint8_t v){ return ((v/10)<<4) | (v%10); }

t_rtc* rtc_init(uint8_t addr)
{
    memset(&g_rtc, 0, sizeof(g_rtc));
    g_rtc.addr    = addr;
    g_rtc.running = true;
    g_rtc.cur_reg = 0;

    // init default time 00:00:00 01/01/25
    g_rtc.regs[0] = 0x00;   // sec
    g_rtc.regs[1] = 0x00;   // min
    g_rtc.regs[2] = 0x00;   // hour
    g_rtc.regs[3] = 0x01;   // day
    g_rtc.regs[4] = 0x01;   // date
    g_rtc.regs[5] = 0x01;   // month
    g_rtc.regs[6] = 0x25;   // year
    g_rtc.regs[7] = 0x00;   // control

    HAL_TIM_Base_Start_IT(rtc_htim);

    return &g_rtc;
}

void rtc_deinit(t_rtc *r)
{
	HAL_TIM_Base_Stop_IT(rtc_htim);
    memset(&g_rtc,0,sizeof(g_rtc));
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
    r->regs[4] = bin2bcd(d->day);
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
    d->day   = bcd2bin(r->regs[4]);
    d->month = bcd2bin(r->regs[5]);
    d->year  = bcd2bin(r->regs[6]);
}

// gọi từ timer 1Hz
void rtc_tick(void)
{
    if (!g_rtc.running) return;

    rtc_time_t t;
    rtc_date_t d;

    rtc_get_time(&g_rtc,&t);
    rtc_get_date(&g_rtc,&d);

    if (++t.sec >= 60){ t.sec = 0;
        if (++t.min >= 60){ t.min = 0;
            if (++t.hour >= 24){ t.hour = 0;
                if (++d.day > 31){ d.day = 1; d.month++; }
                if (d.month > 12){ d.month = 1; d.year++; }
            }
        }
    }

    rtc_set_time(&g_rtc, &t);
    rtc_set_date(&g_rtc, &d);
}


void rtc_addr_handler(I2C_HandleTypeDef *hi2c, uint8_t direction, uint8_t addr)
{
    // nothing
}

void rtc_rx_handler(I2C_HandleTypeDef *hi2c)
{
    uint8_t b;
    HAL_I2C_Slave_Receive(hi2c, &b, 1, 1);

    static bool first = true;

    if (first){
        g_rtc.cur_reg = b;
        first = false;
    } else {
        if (g_rtc.cur_reg < DS1307_REG_COUNT)
            g_rtc.regs[g_rtc.cur_reg++] = b;
    }
}

void rtc_tx_handler(I2C_HandleTypeDef *hi2c)
{
    uint8_t b = g_rtc.regs[g_rtc.cur_reg];
    HAL_I2C_Slave_Transmit(hi2c, &b, 1, 1);

    if (g_rtc.cur_reg < DS1307_REG_COUNT-1)
        g_rtc.cur_reg++;
}

void rtc_listen_handler(I2C_HandleTypeDef *hi2c)
{
	// nothing
}


