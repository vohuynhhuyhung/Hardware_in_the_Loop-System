/*
* USB_CDC.c
*
* Created on: Sep 28, 2025
* Author: VyTran
*
*/

#include <stdbool.h>
#include <stdarg.h>
#include "usbd_cdc_if.h"
#include "cmd.h"

#define RX_BUFFER_SIZE 128

extern USBD_HandleTypeDef hUsbDeviceFS;

static uint8_t usb_rx_buffer[RX_BUFFER_SIZE];
static uint16_t usb_rx_head = 0;
static uint16_t usb_rx_tail = 0;

uint8_t CDC_ReceiveCallback (uint8_t* Buf, uint32_t *Len)
{
    for (uint32_t i = 0; i < *Len; i++) {
        uint16_t next_head = (usb_rx_head + 1) % RX_BUFFER_SIZE;

        if (next_head == usb_rx_tail) {
            usb_rx_tail = (usb_rx_tail + 1) % RX_BUFFER_SIZE;
        }

        usb_rx_buffer[usb_rx_head] = Buf[i];
        usb_rx_head = next_head;
    }

    return USBD_OK;
}

void Console_Init(void) {
    usb_rx_head = 0;
    usb_rx_tail = 0;
    Console_Write("\r\nUSB Console Ready\r\n");
}

void Console_Write(const char *fmt, ...) {
    char buf[128];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len > 0) {
        CDC_Transmit_FS((uint8_t*)buf, len);
    }}

bool Console_Available(void) {
    return usb_rx_head != usb_rx_tail;
}

char Console_Read(void) {
    char c = usb_rx_buffer[usb_rx_tail];
    usb_rx_tail = (usb_rx_tail + 1) % RX_BUFFER_SIZE;
    return c;
}

