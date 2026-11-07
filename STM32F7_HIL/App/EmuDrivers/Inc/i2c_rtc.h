/*
 * i2c_rtc.h
 *
 *  Created on: Nov 1, 2025
 *      Author: VyTran
 */

#ifndef I2C_RTC_H
#define I2C_RTC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f7xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#define DS1307_REG_COUNT   8

typedef struct {
    uint8_t regs[DS1307_REG_COUNT];
    uint8_t addr;
    bool running;
    uint8_t cur_reg;
} t_rtc;

typedef struct {
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
} rtc_time_t;

typedef struct {
	uint8_t day;
	uint8_t month;
	uint8_t year;
} rtc_date_t;

t_rtc* rtc_init(uint8_t addr7);
void   rtc_deinit(t_rtc *r);
void   rtc_tick(void);     // called every 1 sec

t_rtc* find_rtc(uint8_t addr);
void rtc_set_time(t_rtc *r, rtc_time_t *t);
void rtc_set_date(t_rtc *r, rtc_date_t *d);
void rtc_get_time(t_rtc *r, rtc_time_t *t);
void rtc_get_date(t_rtc *r, rtc_date_t *d);

// I2C handlers
void rtc_addr_handler(I2C_HandleTypeDef *hi2c, uint8_t direction, uint8_t addr);
void rtc_rx_handler(I2C_HandleTypeDef *hi2c);
void rtc_tx_handler(I2C_HandleTypeDef *hi2c);
void rtc_listen_handler(I2C_HandleTypeDef *hi2c);

#endif

