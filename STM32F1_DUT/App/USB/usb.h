/*
 * usb.h
 *
 *  Created on: Oct 1, 2025
 *      Author: Samy Ve
 */

#ifndef USB_USB_H_
#define USB_USB_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "usbd_cdc_if.h"

void Console_Init(void);
void Console_Write(const char *fmt, ...);
bool Console_Available(void);
char Console_Read(void);

// Transfer helper APIs
// Để chạy được USB CDC phải gọi api này cuối hàm CDC_Receive_FS trong file usb_cdc_if.c
uint8_t CDC_ReceiveCallback (uint8_t* Buf, uint32_t *Len);

#endif /* USB_USB_H_ */
