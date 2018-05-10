#ifndef APP_H
#define APP_H

#include "tim.h"
#include "platform_iks01a2.h"
#include "x_nucleo_iks01a2_accelero.h"
#include "x_nucleo_iks01a2_gyro.h"
#include "x_nucleo_iks01a2_temperature.h"
#include "x_nucleo_iks01a2_pressure.h"
#include "x_nucleo_iks01a2_humidity.h"
#include "x_nucleo_iks01a2_magneto.h"
#include "sensor.h"

#define FW_VERSION 0
#define FW_SUBVERSION 17

#define REDIRECT_WIFI_TO_USB 0

#define USB_USART huart2

#ifdef USE_STM32L4XX_NUCLEO
  #define USART_DMA_TIM (&htim17)
#else
  #define USART_DMA_TIM (&htim11)
#endif

#define SENSOR_TIM (&htim5)



void app_init();
void app_loop();

extern void *ACCELERO_handle;
extern void *GYRO_handle;
extern void *MAGNETO_handle;
extern void *HUMIDITY_handle;
extern void *TEMPERATURE_handle;
extern void *PRESSURE_handle;

extern float PRESSURE_Value;
extern float HUMIDITY_Value;
extern float TEMPERATURE_Value;
extern SensorAxes_t ACC_Value;
extern SensorAxes_t GYR_Value;
extern SensorAxes_t MAG_Value;



#endif
