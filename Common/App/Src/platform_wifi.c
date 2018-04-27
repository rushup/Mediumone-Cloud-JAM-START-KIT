#include "platform_wifi.h"
#include "platform_usart.h"
#include <atbox/net/wifi/spwf01sa/spwf01sa.h>
#include "app.h"
#include "usart.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

void wifi_event_callback(e_wifi_cb type, char* buff, uint16_t len);
uint8_t wifi_plat_enable_gpio_write(bool state);
bool wifi_plat_send_data(const char* data, uint16_t count);

t_wifi_spwf01sa_handle wifi_handle = {
  .callbacks = {
    .delay_ms = HAL_Delay,
    .get_tick_ms = HAL_GetTick,
    .gpio_write_reset = wifi_plat_enable_gpio_write,
    .usart_send = wifi_plat_send_data,
    .event_callback = wifi_event_callback
  },
  
};

bool wifi_plat_send_data(const char* data, uint16_t len) {
  
  return uart_send(&uart_wifi, data, len);
}



uint16_t new_wifi_data_cb(char* data, uint16_t len)
{
#if REDIRECT_WIFI_TO_USB == 1
  uart_send(&uart_usb, data, len);
#endif
  
  if(wifi_handle.usart_fetch_byte != 0)
  {
    for(int i=0; i < len; i++)
    {
      wifi_handle.usart_fetch_byte(&wifi_handle, data[i]);
    }
  }

  return len;
}

uint8_t wifi_plat_enable_gpio_write(bool state) {
    
  HAL_GPIO_WritePin(WIFI_RST_GPIO_Port, WIFI_RST_Pin, ( GPIO_PinState) state);
  
  return 0;
}

