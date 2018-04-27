
#include "platform_iks01a2.h"
#include "app.h"
#include "tim.h"

static uint32_t I2C_EXPBD_Timeout = NUCLEO_I2C_EXPBD_TIMEOUT_MAX;    /*<! Value of Timeout when I2C communication fails */
extern I2C_HandleTypeDef hi2c1;

/* Link function for sensor peripheral */
uint8_t Sensor_IO_Write( void *handle, uint8_t WriteAddr, uint8_t *pBuffer, uint16_t nBytesToWrite );
uint8_t Sensor_IO_Read( void *handle, uint8_t ReadAddr, uint8_t *pBuffer, uint16_t nBytesToRead );

static void I2C_EXPBD_Error( uint8_t Addr );
static uint8_t I2C_EXPBD_ReadData( uint8_t Addr, uint8_t Reg, uint8_t* pBuffer, uint16_t Size );
static uint8_t I2C_EXPBD_WriteData( uint8_t Addr, uint8_t Reg, uint8_t* pBuffer, uint16_t Size );

void sensors_timer_start()
{
  HAL_TIM_Base_Start_IT(SENSOR_TIM);
}


/**
 * @brief  Configures sensor I2C interface.
 * @param  None
 * @retval COMPONENT_OK in case of success
 * @retval COMPONENT_ERROR in case of failure
 */
DrvStatusTypeDef Sensor_IO_Init( void )
{
  return COMPONENT_OK;
}


/**
 * @brief  Configures sensor interrupts interface for LSM6DSL sensor.
 * @param  None
 * @retval COMPONENT_OK in case of success
 * @retval COMPONENT_ERROR in case of failure
 */
DrvStatusTypeDef LSM6DSL_Sensor_IO_ITConfig( void )
{

  return COMPONENT_OK;
}


  
/**
 * @brief  Configures sensor interrupts interface for LSM303AGR sensor.
 * @param  None
 * @retval COMPONENT_OK in case of success
 * @retval COMPONENT_ERROR in case of failure
 */
DrvStatusTypeDef LSM303AGR_Sensor_IO_ITConfig( void )
{
 
  return COMPONENT_OK;
}

  

/**
 * @brief  Configures sensor interrupts interface for IIS2DH sensor.
 * @param  None
 * @retval COMPONENT_OK in case of success
 * @retval COMPONENT_ERROR in case of failure
 */
DrvStatusTypeDef IIS2DH_Sensor_IO_ITConfig( void )
{

  return COMPONENT_OK;
}



/**
 * @brief  Configures sensor interrupts interface for LPS22HB sensor.
 * @param  None
 * @retval COMPONENT_OK in case of success
 * @retval COMPONENT_ERROR in case of failure
 */
DrvStatusTypeDef LPS22HB_Sensor_IO_ITConfig( void )
{

  return COMPONENT_OK;
}

/**
 * @}
 */


/** @addtogroup X_NUCLEO_IKS01A2_IO_Private_Functions Private functions
 * @{
 */



/******************************* Link functions *******************************/


/**
 * @brief  Writes a buffer to the sensor
 * @param  handle instance handle
 * @param  WriteAddr specifies the internal sensor address register to be written to
 * @param  pBuffer pointer to data buffer
 * @param  nBytesToWrite number of bytes to be written
 * @retval 0 in case of success
 * @retval 1 in case of failure
 */
uint8_t Sensor_IO_Write( void *handle, uint8_t WriteAddr, uint8_t *pBuffer, uint16_t nBytesToWrite )
{
  DrvContextTypeDef *ctx = (DrvContextTypeDef *)handle;

  /* call I2C_EXPBD Read data bus function */
  if ( I2C_EXPBD_WriteData( ctx->address, WriteAddr, pBuffer, nBytesToWrite ) )
  {
    return 1;
  }
  else
  {
    return 0;
  }
}



/**
 * @brief  Reads a from the sensor to buffer
 * @param  handle instance handle
 * @param  ReadAddr specifies the internal sensor address register to be read from
 * @param  pBuffer pointer to data buffer
 * @param  nBytesToRead number of bytes to be read
 * @retval 0 in case of success
 * @retval 1 in case of failure
 */
uint8_t Sensor_IO_Read( void *handle, uint8_t ReadAddr, uint8_t *pBuffer, uint16_t nBytesToRead )
{
  DrvContextTypeDef *ctx = (DrvContextTypeDef *)handle;

  /* call I2C_EXPBD Read data bus function */
  if ( I2C_EXPBD_ReadData( ctx->address, ReadAddr, pBuffer, nBytesToRead ) )
  {
    return 1;
  }
  else
  {
    return 0;
  }
}



/**
 * @brief  Write data to the register of the device through BUS
 * @param  Addr Device address on BUS
 * @param  Reg The target register address to be written
 * @param  pBuffer The data to be written
 * @param  Size Number of bytes to be written
 * @retval 0 in case of success
 * @retval 1 in case of failure
 */
static uint8_t I2C_EXPBD_WriteData( uint8_t Addr, uint8_t Reg, uint8_t* pBuffer, uint16_t Size )
{

  HAL_StatusTypeDef status = HAL_OK;

  status = HAL_I2C_Mem_Write( &hi2c1, Addr, ( uint16_t )Reg, I2C_MEMADD_SIZE_8BIT, pBuffer, Size,
                              I2C_EXPBD_Timeout );

  /* Check the communication status */
  if( status != HAL_OK )
  {

    /* Execute user timeout callback */
    I2C_EXPBD_Error( Addr );
    return 1;
  }
  else
  {
    return 0;
  }
}



/**
 * @brief  Read a register of the device through BUS
 * @param  Addr Device address on BUS
 * @param  Reg The target register address to read
 * @param  pBuffer The data to be read
 * @param  Size Number of bytes to be read
 * @retval 0 in case of success
 * @retval 1 in case of failure
 */
static uint8_t I2C_EXPBD_ReadData( uint8_t Addr, uint8_t Reg, uint8_t* pBuffer, uint16_t Size )
{

  HAL_StatusTypeDef status = HAL_OK;

  status = HAL_I2C_Mem_Read( &hi2c1, Addr, ( uint16_t )Reg, I2C_MEMADD_SIZE_8BIT, pBuffer, Size,
                             I2C_EXPBD_Timeout );

  /* Check the communication status */
  if( status != HAL_OK )
  {

    /* Execute user timeout callback */
    I2C_EXPBD_Error( Addr );
    return 1;
  }
  else
  {
    return 0;
  }
}



/**
 * @brief  Manages error callback by re-initializing I2C
 * @param  Addr I2C Address
 * @retval None
 */
static void I2C_EXPBD_Error( uint8_t Addr )
{

//  /* De-initialize the I2C comunication bus */
//  HAL_I2C_DeInit( &I2C_EXPBD_Handle );
//
//  /* Re-Initiaize the I2C comunication bus */
//  I2C_EXPBD_Init();
}


/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
