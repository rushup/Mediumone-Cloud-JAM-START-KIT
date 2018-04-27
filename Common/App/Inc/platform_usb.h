#ifndef PLATFORM_USB_H
#define PLATFORM_USB_H

#include <stdint.h>

#define USB_DATA_BUFFER_SIZE 2000

extern char usb_data_buffer[USB_DATA_BUFFER_SIZE];

uint16_t new_usb_data_cb(char* data, uint16_t len);
void usb_data_buffer_clear();

#endif