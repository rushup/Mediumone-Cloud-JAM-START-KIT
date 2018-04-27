#include "platform_usb.h"
#include "platform_usart.h"
#include "app.h"

char usb_data_buffer[USB_DATA_BUFFER_SIZE] = {0};
int usb_data_buffer_index = 0;

uint16_t new_usb_data_cb(char* data, uint16_t len)
{
#if REDIRECT_WIFI_TO_USB == 1
  uart_send(&uart_wifi, data, len);
#else
  for(int i=0; i < len; i++)
  {
    if(usb_data_buffer_index < (USB_DATA_BUFFER_SIZE - 2))
      usb_data_buffer[usb_data_buffer_index++] = data[i];
    
    if(data[i] != '\r')
      uart_send(&uart_usb, &data[i], 1);
  }
  
#endif

  
  
  return len;
}

void usb_data_buffer_clear()
{
  memset(usb_data_buffer, 0, USB_DATA_BUFFER_SIZE);
  usb_data_buffer_index = 0;
}