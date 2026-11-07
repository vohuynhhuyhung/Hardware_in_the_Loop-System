/*
 * gpio_control.h
 *
 *  Created on: Apr 3, 2026
 *      Author: Vy Tran
 */

#ifndef EMUDRIVERS_INC_GPIO_CONTROL_H_
#define EMUDRIVERS_INC_GPIO_CONTROL_H_

#include "stm32f4xx_hal.h"
#include "stdint.h"
#include "stdbool.h"
#include "stm32f4xx_ll_gpio.h"

typedef enum
{
    GPIO_OK = 0,
    GPIO_ERR_INVALID,
    GPIO_ERR_DIRECTION
} gpio_status_t;

typedef void (*gpio_list_cb_t)(const char *name,
                               char port,
                               uint8_t pin,
                               const char *dir,
                               uint8_t state);

void gpio_list_all(gpio_list_cb_t cb);
gpio_status_t gpio_write(GPIO_TypeDef *port, uint32_t pin, uint8_t value);
gpio_status_t gpio_read(GPIO_TypeDef *port, uint32_t pin, uint8_t *value);

#endif /* EMUDRIVERS_INC_GPIO_CONTROL_H_ */
