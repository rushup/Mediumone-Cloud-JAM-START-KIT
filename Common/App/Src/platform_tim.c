#include "platform_tim.h"
#include "platform_usart.h"
#include "app.h"
#include "tim.h"

static uint32_t sensor_tim_counter = 0;



void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{

  if(htim->Instance == USART_DMA_TIM->Instance)
  {
    uart_process(&uart_wifi);
    uart_process(&uart_usb);
  }
  else if(htim->Instance == SENSOR_TIM->Instance)
  {

	BSP_ACCELERO_Get_Axes(ACCELERO_handle,&ACC_Value);
	BSP_GYRO_Get_Axes(GYRO_handle,&GYR_Value);
        
        if((sensor_tim_counter % 25) == 0)
        {
          BSP_MAGNETO_Get_Axes(MAGNETO_handle,&MAG_Value);
        }
        
        if((sensor_tim_counter % 100) == 0)
        {
          BSP_TEMPERATURE_Get_Temp(TEMPERATURE_handle,(float *)&TEMPERATURE_Value);
          BSP_HUMIDITY_Get_Hum(HUMIDITY_handle,(float *)&HUMIDITY_Value);
          BSP_PRESSURE_Get_Press(PRESSURE_handle,(float *)&PRESSURE_Value);
        }

        sensor_tim_counter++;
       
  }
}
