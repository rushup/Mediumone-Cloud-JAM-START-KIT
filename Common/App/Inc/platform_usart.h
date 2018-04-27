#ifndef PLATFORM_USART_H
#define PLATFORM_USART_H

#include "usart.h"
#include <stdint.h>
#include <stdbool.h>

#define DMA_USB_BUFFER_SIZE 256
#define DMA_WIFI_BUFFER_SIZE 4096

typedef struct{
  uint32_t RS232_ActDMA;
  uint32_t RS232_PrevDMA;
  uint32_t RS232_RxDataLen;
  uint32_t RS232_RxDataPtrTmp;
  UART_HandleTypeDef* huart;
  char* dma_buffer_rx;
  uint32_t buffer_size;
  uint16_t (*new_data_cb)(char*,uint16_t);
}t_uart_dma_helper;

extern t_uart_dma_helper uart_wifi;
extern t_uart_dma_helper uart_usb;

void uart_init(t_uart_dma_helper* handle);
void uart_process(t_uart_dma_helper* handle);
bool uart_send(t_uart_dma_helper* handle,const char* buffer, uint32_t length);

#endif