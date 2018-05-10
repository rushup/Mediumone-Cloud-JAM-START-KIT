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
//    static char bb[10];
//    snprintf(bb, 10, "%d ", data[i]);
//    uart_send(&uart_usb, bb, strlen(bb));
//    uart_send(&uart_usb, "\r\n", 2);
    
    if(data[i] != '\b' && data[i] != 127) //127 = DEL
    {
      if(usb_data_buffer_index < (USB_DATA_BUFFER_SIZE - 2))
        usb_data_buffer[usb_data_buffer_index++] = data[i];
      
    }else if(usb_data_buffer_index > 0)
    {
      usb_data_buffer[usb_data_buffer_index-1] = 0;
      usb_data_buffer_index--;
    }
    
    if(data[i] != '\r' && data[i] != '\b' && data[i] != 127) //127 = DEL
      uart_send(&uart_usb, &data[i], 1);
    
    /* DEL acts as backspace on screen (linux) */
    if(data[i] == '\b' || data[i] == 127) //127 = DEL
      uart_send(&uart_usb, "\033[D \033[D", strlen("\033[D \033[D"));
  }
  
#endif

  
  
  return len;
}


void usb_data_buffer_clear()
{
  memset(usb_data_buffer, 0, USB_DATA_BUFFER_SIZE);
  usb_data_buffer_index = 0;
}