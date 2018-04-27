#include "platform_usart.h"
#include "platform_usb.h"
#include "platform_wifi.h"
#include "app.h"
#include "usart.h"
#include <stdint.h>
#include <stdbool.h>

char dma_buffer_wifi_rx[DMA_WIFI_BUFFER_SIZE];
char dma_buffer_usb_rx[DMA_USB_BUFFER_SIZE];

t_uart_dma_helper uart_wifi = {.huart = &huart1,.buffer_size = DMA_WIFI_BUFFER_SIZE,.dma_buffer_rx = dma_buffer_wifi_rx, .new_data_cb = new_wifi_data_cb};
t_uart_dma_helper uart_usb = {.huart = &huart2,.buffer_size = DMA_USB_BUFFER_SIZE,.dma_buffer_rx = dma_buffer_usb_rx, .new_data_cb = new_usb_data_cb};


void uart_dma_helper_init(t_uart_dma_helper* handle);
void uart_dma_helper_process(t_uart_dma_helper* handle);
bool uart_dma_helper_send(t_uart_dma_helper* handle,uint8_t* buffer, uint32_t length);


void uart_init(t_uart_dma_helper* handle)
{
  HAL_UART_Receive_DMA(handle->huart,(uint8_t*) handle->dma_buffer_rx,handle->buffer_size);
  
  HAL_TIM_Base_Start_IT(USART_DMA_TIM);
}

void uart_process(t_uart_dma_helper* handle)
{
#ifdef USE_STM32L4XX_NUCLEO
  handle->RS232_ActDMA = handle->huart->hdmarx->Instance->CNDTR;
#else
  handle->RS232_ActDMA = handle->huart->hdmarx->Instance->NDTR;
#endif
  if(handle->RS232_ActDMA != handle->RS232_PrevDMA)
  {
    if(handle->RS232_ActDMA == 0)
    {
      return;
    }
    if(handle->RS232_ActDMA < handle->RS232_PrevDMA)
    {
      handle->RS232_RxDataLen = handle->RS232_PrevDMA - handle->RS232_ActDMA;
    }
    else
    {
      handle->RS232_RxDataLen = ((handle->buffer_size - handle->RS232_ActDMA) + handle->RS232_PrevDMA);
    }
    handle->RS232_RxDataPtrTmp = (handle->buffer_size - handle->RS232_PrevDMA);//%handle->buffer_size;
    handle->RS232_PrevDMA = handle->RS232_ActDMA;
    
    while(handle->RS232_RxDataLen)
    {

      handle->new_data_cb(&handle->dma_buffer_rx[handle->RS232_RxDataPtrTmp],1);
      
      handle->RS232_RxDataPtrTmp++;
      if(handle->RS232_RxDataPtrTmp >= handle->buffer_size)
      {
        handle->RS232_RxDataPtrTmp = 0;
      }
      handle->RS232_RxDataLen--;
    }
  }
}

bool uart_send(t_uart_dma_helper* handle, const char* buffer, uint32_t length)
{
//  uint32_t tick;
//  tick = HAL_GetTick();
//  while(handle->huart->gState != HAL_UART_STATE_READY)
//  {
//    if((HAL_GetTick() - tick) > 2000)
//      return false;
//  }
//  
  return (HAL_UART_Transmit(handle->huart,(uint8_t*)buffer,length, 1000) == HAL_OK);
}

