/*
 * gpio_control.c
 *
 *  Created on: Apr 3, 2026
 *      Author: Vy Tran
 */

#include "gpio_control.h"

typedef enum
{
    GPIO_DIR_INPUT,
    GPIO_DIR_OUTPUT
} gpio_dir_t;

typedef struct
{
    GPIO_TypeDef *port;
    uint32_t pin;
    gpio_dir_t dir;
} gpio_entry_t;

static const gpio_entry_t gpio_table[] =
{
    /* ================= GPIOE INPUT ================= */
    {GPIOE, LL_GPIO_PIN_0,  GPIO_DIR_INPUT},
    {GPIOE, LL_GPIO_PIN_1,  GPIO_DIR_INPUT},
    {GPIOE, LL_GPIO_PIN_2,  GPIO_DIR_INPUT},
    {GPIOE, LL_GPIO_PIN_3,  GPIO_DIR_INPUT},
    {GPIOE, LL_GPIO_PIN_4,  GPIO_DIR_INPUT},
    {GPIOE, LL_GPIO_PIN_5,  GPIO_DIR_INPUT},
    {GPIOE, LL_GPIO_PIN_6,  GPIO_DIR_INPUT},
    {GPIOE, LL_GPIO_PIN_7,  GPIO_DIR_INPUT},
    {GPIOE, LL_GPIO_PIN_8,  GPIO_DIR_INPUT},
    {GPIOE, LL_GPIO_PIN_9,  GPIO_DIR_INPUT},
    {GPIOE, LL_GPIO_PIN_10, GPIO_DIR_INPUT},
    {GPIOE, LL_GPIO_PIN_11, GPIO_DIR_INPUT},
    {GPIOE, LL_GPIO_PIN_12, GPIO_DIR_INPUT},
    {GPIOE, LL_GPIO_PIN_13, GPIO_DIR_INPUT},

    /* ================= GPIOB OUTPUT ================= */
    {GPIOB, LL_GPIO_PIN_0,  GPIO_DIR_OUTPUT},
    {GPIOB, LL_GPIO_PIN_1,  GPIO_DIR_OUTPUT},
    {GPIOB, LL_GPIO_PIN_2,  GPIO_DIR_OUTPUT},
    {GPIOB, LL_GPIO_PIN_3,  GPIO_DIR_OUTPUT},
    {GPIOB, LL_GPIO_PIN_4,  GPIO_DIR_OUTPUT},
    {GPIOB, LL_GPIO_PIN_5,  GPIO_DIR_OUTPUT},
    {GPIOB, LL_GPIO_PIN_10, GPIO_DIR_OUTPUT},
    {GPIOB, LL_GPIO_PIN_11, GPIO_DIR_OUTPUT},
    {GPIOB, LL_GPIO_PIN_12, GPIO_DIR_OUTPUT},
    {GPIOB, LL_GPIO_PIN_13, GPIO_DIR_OUTPUT},
    {GPIOB, LL_GPIO_PIN_14, GPIO_DIR_OUTPUT},
    {GPIOB, LL_GPIO_PIN_15, GPIO_DIR_OUTPUT},
};

#define GPIO_TABLE_SIZE (sizeof(gpio_table)/sizeof(gpio_table[0]))

static char port_to_char(GPIO_TypeDef *port)
{
    if(port == GPIOA) return 'A';
    if(port == GPIOB) return 'B';
    if(port == GPIOC) return 'C';
    if(port == GPIOD) return 'D';
    if(port == GPIOE) return 'E';
    return '?';
}

static uint8_t pin_to_number(uint32_t pin)
{
    for(uint8_t i = 0; i < 16; i++)
    {
        if(pin == (1U << i))
            return i;
    }
    return 255;
}

static const gpio_entry_t* gpio_find(GPIO_TypeDef *port, uint32_t pin)
{
    for(uint32_t i = 0; i < GPIO_TABLE_SIZE; i++)
    {
        if(gpio_table[i].port == port &&
           gpio_table[i].pin  == pin)
            return &gpio_table[i];
    }
    return NULL;
}

gpio_status_t gpio_write(GPIO_TypeDef *port, uint32_t pin, uint8_t value)
{
    const gpio_entry_t *g = gpio_find(port, pin);

    if(!g) return GPIO_ERR_INVALID;
    if(g->dir != GPIO_DIR_OUTPUT) return GPIO_ERR_DIRECTION;

    if(value) {
        LL_GPIO_SetOutputPin(port, pin);
    }
    else {
        LL_GPIO_ResetOutputPin(port, pin);
    }

    return GPIO_OK;
}

gpio_status_t gpio_read(GPIO_TypeDef *port, uint32_t pin, uint8_t *value)
{
    const gpio_entry_t *g = gpio_find(port, pin);

    if(!g) return GPIO_ERR_INVALID;

    if(g->dir == GPIO_DIR_INPUT) {
    	*value = LL_GPIO_IsInputPinSet(port, pin);
    }
    else {
    	*value = LL_GPIO_IsOutputPinSet(port, pin);
    }

    return GPIO_OK;
}

void gpio_list_all(gpio_list_cb_t cb)
{
    for(uint32_t i = 0; i < GPIO_TABLE_SIZE; i++)
    {
        const gpio_entry_t *g = &gpio_table[i];

        char port = port_to_char(g->port);
        uint8_t pin = pin_to_number(g->pin);

        const char *dir = (g->dir == GPIO_DIR_INPUT) ? "IN" : "OUT";

        uint8_t state;

        if(g->dir == GPIO_DIR_INPUT) {
            state = LL_GPIO_IsInputPinSet(g->port, g->pin);
        }
        else {
            state = LL_GPIO_IsOutputPinSet(g->port, g->pin);
        }

        cb(NULL, port, pin, dir, state);
    }
}
