/*
 * gpio_cmd.c
 *
 *  Created on: Apr 3, 2026
 *      Author: Vy Tran
 */

#include <stdio.h>
#include <stdlib.h>
#include "cmd.h"
#include "gpio_control.h"

static GPIO_TypeDef* parse_port(char p)
{
    switch(p)
    {
        case 'A':
        case 'a': return GPIOA;
        case 'B':
        case 'b': return GPIOB;
        case 'C':
        case 'c': return GPIOC;
        case 'D':
        case 'd': return GPIOD;
        case 'E':
        case 'e': return GPIOE;
        default:  return NULL;
    }
}

static void gpio_print_cb(const char *name,
                          char port,
                          uint8_t pin,
                          const char *dir,
                          uint8_t state)
{
    Console_Write("%-4c %-4u %-5s %-5u\r\n",
                  port,
                  pin,
                  dir,
                  state);
}

static int Cmd_GPIO_List(int argc, char *argv[])
{
	if (argc != 2)
	    return (argc < 2) ? CMDLINE_TOO_FEW_ARGS : CMDLINE_TOO_MANY_ARGS;

    Console_Write("\r\n%-4s %-4s %-5s %-5s\r\n", "PORT", "PIN", "DIR", "STATE");
    Console_Write("---------------------\r\n");
    gpio_list_all(gpio_print_cb);

    return CMDLINE_OK;
}

static int Cmd_GPIO_Write(int argc, char *argv[])
{
    if(argc != 5)
        return (argc < 5) ? CMDLINE_TOO_FEW_ARGS : CMDLINE_TOO_MANY_ARGS;

    GPIO_TypeDef *port = parse_port(argv[1][0]);
    uint32_t pin_num = atoi(argv[2]);
    uint32_t value   = atoi(argv[3]);

    if(port == NULL || pin_num > 15) {
    	Console_Write("Invalid GPIO port/pin");
    	return CMDLINE_INVALID_ARG;
    }

    uint32_t pin = (1U << pin_num);

    if(gpio_write(port, pin, value) != GPIO_OK)
    {
        Console_Write("GPIO write failed\r\n");
        return CMDLINE_EXEC_FAILED;
    }

    Console_Write("GPIO write OK\r\n");
    return CMDLINE_OK;
}

static int Cmd_GPIO_Read(int argc, char *argv[])
{
    if(argc != 4)
        return (argc < 4) ? CMDLINE_TOO_FEW_ARGS : CMDLINE_TOO_MANY_ARGS;

    GPIO_TypeDef *port = parse_port(argv[1][0]);
    uint32_t pin_num = atoi(argv[2]);

    if(port == NULL || pin_num > 15)
        return CMDLINE_INVALID_ARG;

    uint32_t pin = (1U << pin_num);
    uint8_t value;
    if(gpio_read(port, pin, &value) != GPIO_OK)
    {
        Console_Write("GPIO read failed\r\n");
        return CMDLINE_EXEC_FAILED;
    }

    Console_Write("GPIO = %d\r\n", value);
    return CMDLINE_OK;
}

void Cmd_GPIO_Register(void)
{
    CLI_RegisterCommand("gpio_list", Cmd_GPIO_List, "List available GPIO");
    CLI_RegisterCommand("gpio_write",  Cmd_GPIO_Write, "gpio_write <port> <pin> <0|1>");
    CLI_RegisterCommand("gpio_read",  Cmd_GPIO_Read, "gpio_read <port> <pin>");
}

