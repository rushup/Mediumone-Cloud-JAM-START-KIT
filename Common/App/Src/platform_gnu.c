#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "usart.h"
#include "app.h"
#include "platform_usart.h."

#define UartHandle USB_USART

void myprintf(char * format, ...)
{
  char buffer[2048];
  va_list args;
  va_start (args, format);
  vsnprintf (buffer, 2047, format, args);

  HAL_UART_Transmit(&UartHandle, (uint8_t *)buffer, strlen(buffer), HAL_MAX_DELAY);

  va_end (args);
}

