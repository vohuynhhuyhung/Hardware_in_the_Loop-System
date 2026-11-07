/*
 * uart_driver.c
 *
 *  Created on: Feb 18, 2026
 *      Author: VyTran
 */
#include "uart_driver.h"
#include <string.h>

extern UART_HandleTypeDef huart2;
UART_HandleTypeDef *uart = &huart2;

static uint8_t uart_rx_byte;
static uint8_t uart_rx_buffer[UART_RX_BUF_LEN];
volatile uint16_t uart_rx_head = 0;
volatile uint16_t uart_rx_tail = 0;
volatile uint8_t uart_overflow = 0;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == uart)
    {
        uint16_t next = (uart_rx_head + 1) % UART_RX_BUF_LEN;

        if (next != uart_rx_tail)
        {
            uart_rx_buffer[uart_rx_head] = uart_rx_byte;
            uart_rx_head = next;
        }
        else
        {
            uart_overflow = 1;
        }

        HAL_UART_Receive_IT(uart, &uart_rx_byte, 1);
    }
}

void UART_Init(void)
{
    HAL_UART_Receive_IT(uart, &uart_rx_byte, 1);
}

uint16_t UART_Available(void)
{
    if (uart_rx_head >= uart_rx_tail) {
        return uart_rx_head - uart_rx_tail;
    } else {
        return UART_RX_BUF_LEN - uart_rx_tail + uart_rx_head;
    }
}

uint16_t UART_Read(uint8_t *buf, uint16_t len)
{
    uint16_t count = 0;

    while ((count < len) && (uart_rx_head != uart_rx_tail))
    {
        buf[count++] = uart_rx_buffer[uart_rx_tail];
        uart_rx_tail = (uart_rx_tail + 1) % UART_RX_BUF_LEN;
    }

    return count;
}

void UART_Clear(void)
{
    uart_rx_head = uart_rx_tail = 0;
    uart_overflow = 0;
}

void UART_Send(uint8_t *data, uint16_t len)
{
    HAL_UART_Transmit(uart, data, len, 1000);
}

void UART_Send_String(const char *str)
{
    HAL_UART_Transmit(uart, (uint8_t*)str, strlen(str), 1000);
}
