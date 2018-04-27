#include "platform_mqtt.h"
#ifdef USE_STM32L4XX_NUCLEO
  #include "stm32l4xx_hal.h"
#else
  #include "stm32f4xx_hal.h"
#endif
#include "platform_wifi.h"
#include "MQTTClient.h"



static uint16_t  sock_id;

// all failure return codes must be negative, as per MQTT Paho
typedef enum
{ 
  MQTT_SUCCESS  = 1,
  MQTT_ERROR    = -1 
} Mqtt_Status_t;

Mqtt_Status_t mqtt_intf_status;


/**
  * @brief  Check timeout 
  * @param  timer : Time structure
  * @retval 1 for time expired, 0 otherwise
  */
char expired(Timer* timer)
{
      long left = timer->end_time - HAL_GetTick();
     // printf("left %ld \r\n", left);  
      
      if(left < 0)
      {
        asm("nop");
      }

      return (left < 0);
}

/**
  * @brief  Countdown in ms 
  * @param  timer : Time structure, timout
  * @retval None
  */
void countdown_ms(Timer* timer, unsigned int timeout)
{
	timer->end_time = HAL_GetTick() + timeout;
}

/**
  * @brief  Countdown in us 
  * @param  timer : Time structure, timout
  * @retval None
  */
void countdown(Timer* timer, unsigned int timeout)
{
	timer->end_time = HAL_GetTick() + (timeout * 1000);
}

/**
  * @brief  Milliseconds left before timeout
  * @param  timer : Time structure
  * @retval Left milliseconds
  */
int left_ms(Timer* timer)
{
	long left = timer->end_time - HAL_GetTick();
	return (left < 0) ? 0 : left;
}

/**
  * @brief  Init timer structure
  * @param  timer : Time structure
  * @retval None
  */
void InitTimer(Timer* timer)
{
	timer->end_time = 0;
}

/**
  * @brief  Connect with MQTT Broker via TCP or TLS via anonymous authentication
  * @param  net : Network structure
  *         hostname : server name 
  *         port_number : server port number 
  *         protocol : 't' for TCP, 's' for TLS
  * @retval Status : OK/ERR
  */
int mqtt_open_socket(Network* net, char* hostname, uint32_t port_number, char protocol)
{
    e_wifi_ris ris;
    ris = spwf01sa_open(&wifi_handle, hostname, port_number, protocol, &sock_id);
    net->my_socket = sock_id;
    
    if(ris == WIFI_OK)
      return MQTT_SUCCESS;
    else
      return MQTT_ERROR;

 }
 
/**
  * @brief  Close socket 
  * @param  n : Network structure
  * @retval None
  */
void mqtt_socket_close (Network* n)
{
  spwf01sa_close(&wifi_handle, n->my_socket);
}

/**
  * @brief  Socket write 
  * @param  net : Network structure
  *         buffer : buffer to write
  *         lenBuf : lenght data in buffer 
  *         timeout : timeout for writing to socket  
  * @retval lenBuff : number of bytes written to socket 
  */
int mqtt_socket_write (Network* net, unsigned char* buffer, int lenBuf, int timeout)
{
   mqtt_intf_status = MQTT_ERROR;
   e_wifi_ris ris;
   ris = spwf01sa_send(&wifi_handle, net->my_socket, (char*) buffer, lenBuf);
   
   if(ris != WIFI_OK)
     return MQTT_ERROR;
 
   return lenBuf;
 }

/**
  * @brief  Wrapping function to read data from an buffer queue, which is filled by WiFi callbacks 
  * @param  net : Network structure 
  *         i : buffer to fill with data read
  *         ch : number of bytes to read
  *         timeout : timeout for writing to socket 
  * @retval sizeReceived : number of bytes read
  */
int mqtt_socket_read (Network* net, unsigned char* i, int ch, int timeout)
{
  /*Recv works only if the buffer is equal or larger then data received, so we need to buffer the data */
  static char socket_buffer[2000] = {0};
  static int socket_buffer_index = 0;
  
  uint16_t recv_len = 0;
  
   e_wifi_ris ris;
   

   ris = spwf01sa_recv(&wifi_handle, net->my_socket, socket_buffer + socket_buffer_index, 2000, &recv_len);
   
   
   if(ris != WIFI_OK)
   {
     return MQTT_ERROR;
   }
   
   socket_buffer_index += recv_len;

   

   if(socket_buffer_index > 0 && ch > 0)
   {
     if((ch) <= socket_buffer_index)
     {
       memcpy(i, socket_buffer, ch);
       memcpy(&socket_buffer, socket_buffer + ch , 2000 - ch);
       
       socket_buffer_index -= ch;
       recv_len = ch;
     }
     else
     {
       recv_len = socket_buffer_index;
       socket_buffer_index = 0;
       memcpy(i, socket_buffer, recv_len);
       memcpy(&socket_buffer, socket_buffer + recv_len , 2000 - recv_len);
       
     }
     
   }
   else
     return 0;


 
//  sizeReceived = LocalBufferGetSizeBuffer(&localBufferReading);
//  
//  if(sizeReceived > 0) 
//  {    
//    if(sizeReceived >= ch)
//    {
//      LocalBufferPopBuffer(&localBufferReading, i, ch);
//      return ch;
//    }
//    else
//    {  
//      LocalBufferPopBuffer(&localBufferReading, i, sizeReceived);
//      return sizeReceived;
//    }     
//  }
  return recv_len;
 }

/**
  * @brief  Map MQTT network interface with SPWF wrapper functions 
  * @param  n : Network structure 
  * @retval None
  */
void NewNetwork(Network* n)
{
	n->my_socket =0;
	n->mqttread = mqtt_socket_read;
	n->mqttwrite = mqtt_socket_write;
        n->disconnect = mqtt_socket_close;
}

