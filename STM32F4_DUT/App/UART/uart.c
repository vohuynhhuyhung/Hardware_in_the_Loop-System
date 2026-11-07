/*
 * uart.c
 *
 *  Created on: Sep 21, 2025
 *      Author: Samy Ve
 */

#include "uart.h"
#include <string.h>
#include "stm32f4xx_ll_gpio.h"
#include "main.h"

USART_TypeDef *usart1 = USART1;				//Debug Interface

#define TIMEOUT_DEF 500  // 500ms timeout
uint16_t timeout;

#define ATOMIC_BLOCK_START(uart) LL_USART_DisableIT_RXNE(uart)
#define ATOMIC_BLOCK_END(uart)   LL_USART_EnableIT_RXNE(uart)

typedef struct {
    USART_TypeDef *uart;
    ring_buffer rx_buffer;
    ring_buffer tx_buffer;
} USART_Buffer;

#define USART_COUNT 1

unsigned char usart1_rx_data[USART1_BUFFER_SIZE];
unsigned char usart1_tx_data[USART1_BUFFER_SIZE];

USART_Buffer usart_buffers[USART_COUNT] = {
    {
        USART1,
        { usart1_rx_data, 0, 0, USART1_BUFFER_SIZE },
        { usart1_tx_data, 0, 0, USART1_BUFFER_SIZE }
    }
};

void store_char(unsigned char c, ring_buffer *buffer, USART_TypeDef *uart);

USART_Buffer* get_usart_buffer(USART_TypeDef *uart) {
    for (int i = 0; i < USART_COUNT; i++) {
        if (usart_buffers[i].uart == uart) {
            return &usart_buffers[i];
        }
    }
    return NULL;
}

void UART_RingBuffer_Init(void) {
    for (int i = 0; i < USART_COUNT; i++) {
        USART_Buffer *buffer = &usart_buffers[i];

        memset(buffer->rx_buffer.buffer, 0, buffer->rx_buffer.size);
        buffer->rx_buffer.head = 0;
        buffer->rx_buffer.tail = 0;

        memset(buffer->tx_buffer.buffer, 0, buffer->tx_buffer.size);
        buffer->tx_buffer.head = 0;
        buffer->tx_buffer.tail = 0;

        LL_USART_EnableIT_ERROR(buffer->uart);
        LL_USART_EnableIT_RXNE(buffer->uart);
    }
}

void store_char(unsigned char c, ring_buffer *buffer, USART_TypeDef *uart) {
    int i = (buffer->head + 1) % buffer->size;

    if (i != buffer->tail) {
        ATOMIC_BLOCK_START(uart);
        buffer->buffer[buffer->head] = c;
        buffer->head = i;
        ATOMIC_BLOCK_END(uart);
    }
}

int UART_ReadRing(USART_TypeDef *uart) {
    USART_Buffer *buffer = get_usart_buffer(uart);
    if (!buffer) return -1;

    ring_buffer *rx_buffer = &buffer->rx_buffer;

    if (rx_buffer->head == rx_buffer->tail) {
        return -1;
    } else {
        ATOMIC_BLOCK_START(uart);
        unsigned char c = rx_buffer->buffer[rx_buffer->tail];
        rx_buffer->tail = (rx_buffer->tail + 1) % rx_buffer->size;
        ATOMIC_BLOCK_END(uart);
        return c;
    }
}

void UART_WriteRing(USART_TypeDef *uart, int c) {
    USART_Buffer *buffer = get_usart_buffer(uart);
    if (!buffer || c < 0) return;

    ring_buffer *tx_buffer = &buffer->tx_buffer;
    int i = (tx_buffer->head + 1) % tx_buffer->size;

    ATOMIC_BLOCK_START(uart);
    while (i == tx_buffer->tail);

    tx_buffer->buffer[tx_buffer->head] = (uint8_t)c;
    tx_buffer->head = i;
    ATOMIC_BLOCK_END(uart);

    LL_USART_EnableIT_TXE(uart);
}

/* checks if the new data is available in the incoming buffer
 */
int IsDataAvailable(USART_TypeDef *uart) {
    USART_Buffer *buffer = get_usart_buffer(uart);
    if (!buffer) return 0;

    ring_buffer *rx_buffer = &buffer->rx_buffer;
    return (uint16_t)(rx_buffer->size + rx_buffer->head - rx_buffer->tail) % rx_buffer->size;
}
/* sends the string to the uart
 */
void UART_SendStringRing (USART_TypeDef *uart, const char *s)
{
	while(*s) UART_WriteRing(uart, *s++);
}

//This function could act same as Printf but performance very bad, not recommend using this
//#include <stdarg.h>
//
//void UART_PrintfStringRing(UART_HandleTypeDef *huart, const char *format, ...) {
//    char buffer[128];
//    va_list args;
//    va_start(args, format);
//    vsnprintf(buffer, sizeof(buffer), format, args);
//    va_end(args);
//
//    UART_SendStringRing(huart, buffer);
//}

void UART_Flush_RingRx(USART_TypeDef *uart) {
    USART_Buffer *buffer = get_usart_buffer(uart);
    if (!buffer) return;

    ring_buffer *rx_buffer = &buffer->rx_buffer;
    memset(rx_buffer->buffer, '\0', rx_buffer->size);
    rx_buffer->head = 0;
    rx_buffer->tail = 0;
}

void UART_Flush_RingTx(USART_TypeDef *uart) {
    USART_Buffer *buffer = get_usart_buffer(uart);
    if (!buffer) return;

    ring_buffer *tx_buffer = &buffer->tx_buffer;
    memset(tx_buffer->buffer, '\0', tx_buffer->size);
    tx_buffer->head = 0;
    tx_buffer->tail = 0;
}

void UART_Ring_ISR(USART_TypeDef *uart) {
    USART_Buffer *buffer = get_usart_buffer(uart);
    if (!buffer) return;

    ring_buffer *rx_buffer = &buffer->rx_buffer;
    ring_buffer *tx_buffer = &buffer->tx_buffer;

    if ((LL_USART_IsActiveFlag_RXNE(uart) != RESET) && (LL_USART_IsEnabledIT_RXNE(uart) != RESET)) {
        unsigned char data = LL_USART_ReceiveData8(uart);

        if ((LL_USART_IsActiveFlag_ORE(uart) != RESET) ||
            (LL_USART_IsActiveFlag_FE(uart) != RESET) ||
            (LL_USART_IsActiveFlag_NE(uart) != RESET)) {
            LL_USART_ClearFlag_ORE(uart);
            LL_USART_ClearFlag_FE(uart);
            LL_USART_ClearFlag_NE(uart);
        } else {
        	store_char(data, rx_buffer, uart);
        }
        return;
    }

    if ((LL_USART_IsActiveFlag_TXE(uart) != RESET) && (LL_USART_IsEnabledIT_TXE(uart) != RESET)) {
        if (tx_buffer->head == tx_buffer->tail) {
            LL_USART_DisableIT_TXE(uart);
        } else {
            unsigned char c = tx_buffer->buffer[tx_buffer->tail];
            tx_buffer->tail = (tx_buffer->tail + 1) % tx_buffer->size;
            LL_USART_TransmitData8(uart, c);
        }
    }
}

