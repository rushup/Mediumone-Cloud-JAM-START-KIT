#include "app.h"
#include "i2c.h"
#include "platform_mqtt.h"
#include "platform_wifi.h"
#include "platform_usart.h"
#include "lib_TagType4.h"
#include "m1_mouserkit.h"
#include "platform_gnu.h"


void *ACCELERO_handle = NULL;
void *GYRO_handle = NULL;
void *MAGNETO_handle = NULL;
void *HUMIDITY_handle = NULL;
void *TEMPERATURE_handle = NULL;
void *PRESSURE_handle = NULL;

float PRESSURE_Value;
float HUMIDITY_Value;
float TEMPERATURE_Value;
SensorAxes_t ACC_Value;
SensorAxes_t GYR_Value;
SensorAxes_t MAG_Value;

#define MQTT_BUFFER_SIZE                2048
unsigned char MQTT_read_buf[MQTT_BUFFER_SIZE];
unsigned char MQTT_write_buf[MQTT_BUFFER_SIZE];
char json_buffer[MQTT_BUFFER_SIZE];
MQTTPacket_connectData options = MQTTPacket_connectData_initializer; 
Network network;
Client  client;
MQTTMessage  MQTT_msg;
void messageArrived(MessageData* md);
typedef enum{
  mqtt_socket_create,
  mqtt_connect,
  mqtt_sub,
  mqtt_pub,
  mqtt_pub_aggr,
  mqtt_yield
}e_wifi_state;

enum { C_ACCEPTED = 0, C_REFUSED_PROT_V = 1, C_REFUSED_ID = 2, C_REFUSED_SERVER = 3, C_REFUSED_UID_PWD = 4, C_REFUSED_UNAUTHORIZED = 5 };


static bool sensors_init(void)
{
  if(BSP_ACCELERO_Init( LSM6DSL_X_0, &ACCELERO_handle)!=COMPONENT_OK){
    /* otherwise try to use LSM6DS on board */
    return false;
  }
  /* gyro sensor */
  if(BSP_GYRO_Init( GYRO_SENSORS_AUTO, &GYRO_handle )!=COMPONENT_OK){
    return false;
  }
  
  /* magneto sensor */
  if(BSP_MAGNETO_Init( MAGNETO_SENSORS_AUTO, &MAGNETO_handle )!=COMPONENT_OK){
    return false;
  }
  
  /* Force to use HTS221 */
  if(BSP_HUMIDITY_Init( HTS221_H_0, &HUMIDITY_handle )!=COMPONENT_OK){
    return false;
  }
  
  /* Force to use HTS221 */
  if(BSP_TEMPERATURE_Init( HTS221_T_0, &TEMPERATURE_handle )!=COMPONENT_OK){
    return false;
  }
  
  /* Try to use LPS25HB DIL24 if present, otherwise use LPS25HB on board */
  if(BSP_PRESSURE_Init( PRESSURE_SENSORS_AUTO, &PRESSURE_handle )!=COMPONENT_OK){
    return false;
  }
  
  
  if(BSP_ACCELERO_Set_ODR_Value( ACCELERO_handle, 416.0f ) != COMPONENT_OK)
  {
    return false;
  }
  
  if(BSP_GYRO_Set_ODR_Value( ACCELERO_handle, 416.0f ) != COMPONENT_OK)
  {
    return false;
  }
  
  /*  Enable all the sensors */
  BSP_ACCELERO_Sensor_Enable( ACCELERO_handle );
  BSP_GYRO_Sensor_Enable( GYRO_handle );
  BSP_MAGNETO_Sensor_Enable( MAGNETO_handle );
  BSP_HUMIDITY_Sensor_Enable( HUMIDITY_handle );
  BSP_TEMPERATURE_Sensor_Enable( TEMPERATURE_handle );
  BSP_PRESSURE_Sensor_Enable( PRESSURE_handle );
  
  return true;
}


bool nfc_init()
{
  int initFailCount = 0;
  /* Initialize the M24SR component */
  while (TT4_Init() != SUCCESS){
    initFailCount++;
    if(initFailCount==5) {
      return false;
    }
  }
  
  return true;
}

static bool nfc_write_url(char* UrlForNFC)
{
  sURI_Info URI;
  
  /* Write URI */
  strcpy(URI.protocol,URI_ID_0x03_STRING);
  strcpy(URI.URI_Message,(char *)UrlForNFC);
  strcpy(URI.Information,"\0");
  
  if(TT4_WriteURI(&URI) == SUCCESS) {
    myprintf("Written URL in NFC\r\n");
    myprintf((char *)UrlForNFC);
    myprintf("\r\n");
    return true;
  }
  
  return false;
}

static bool InitMQTT(void)
{
  /* Initialize network interface for MQTT  */
  NewNetwork(&network);
  
  return true;
} 


static bool wifi_init()
{
  bool do_factory_reset = false;
  
  e_wifi_ris ris;
  
  char version[50];
  
  spwf01sa_setup(&wifi_handle);
  
  HAL_GPIO_WritePin(WIFI_WAKEUP_GPIO_Port, WIFI_WAKEUP_Pin, GPIO_PIN_SET);
  
redo:
  
  ris = spwf01sa_init(&wifi_handle);
  
  if(ris != WIFI_OK)
    return false;
  
  if(do_factory_reset == true)
  {
    ris = spwf01sa_factory_reset(&wifi_handle);
    if(ris == WIFI_OK)
    {
      printf("WiFi Factory reset successful\r\n");
      do_factory_reset = false;
      goto redo;
    }
    else
    {
      printf("Wifi Factory reset FAIL \r\n");
      return ris;
    }
    
  }

  ris = spwf01sa_wait_ready(&wifi_handle);

  if(ris == WIFI_TIMEOUT)
  {
    /* If other hardware have bad configuration saved, we need to reboot and send factory reset after power on*/
    printf("WiFi HWReady not detected, trying recovery... \r\n");
    do_factory_reset = true;
    
    goto redo;
  }

  ris = spwf01sa_get_version(&wifi_handle, version, 50);

  if(ris != WIFI_OK)
    return false;
  
  return true;
}

void app_init()
{
  char mac[30] = "00:00:00:00:00:00";
  e_wifi_ris ris;
  t_wifi_ap ap;
  
  myprintf("\033[2J\033[0;0H******************************************************************************\r\n\n\r");
  myprintf("**************************************************************************\r\n");
  myprintf("***                   MEDIUMONE CLOUD JAM DEMO V%d.%d                    ***\r\n", FW_VERSION, FW_SUBVERSION);
  myprintf("**************************************************************************\n\r");

  if(nfc_init() == false)
  {
	  myprintf("NFC Fail\r\n");
  }
  else
    nfc_write_url("mediumone.com");
  
  if(sensors_init() == false)
  {
	  myprintf("Sensors init fail \r\n");
    while(1);
  }
  
  uart_init(&uart_wifi);
  uart_init(&uart_usb);
  

  
  if(wifi_init() == false)
  {
	  myprintf("Wifi init fail \r\n");
    while(1);
  }

  spwf01sa_get_config(&wifi_handle, "nv_wifi_macaddr", mac, sizeof(mac));
  
  sensors_timer_start();
  
  m1_init();
  
  InitMQTT();
  
  m1_config_mqtt((uint8_t*) mac);

redo:
  if(strlen(m1_net_config.ssid) > 0 && strlen(m1_net_config.password) > 0)
  {
    myprintf("Searching WiFi %s \r\n", m1_net_config.ssid);
    
    ris = spwf01sa_scan(&wifi_handle, m1_net_config.ssid, &ap);
    
    if(ris == WIFI_OK && strcmp(ap.ssid, m1_net_config.ssid) == 0)
    {
      myprintf("Network found \r\n");

      
      myprintf("Using password %s \r\n", m1_net_config.password);
      strcpy(ap.psw, m1_net_config.password);
      ris = spwf01sa_connect(&wifi_handle, &ap);
      
      if(ris == WIFI_OK)
      {
        myprintf("Connection successuful \r\n");
      }
      else
      {
        myprintf("Connection failed \r\n");
        goto redo;
      }
    }
    else
    {
      myprintf("Network not found\r\n");
      goto redo;
    }
  }
  
  
  
}

void wifi_event_callback(e_wifi_cb type, char* buff, uint16_t len)
{
  
}

void app_loop()
{
  static e_wifi_state wifi_state = mqtt_socket_create;
  static int ret_code = 0;
  static uint32_t state_tick = 0, state_tick_aggr = 0;
  
  switch (wifi_state) 
  {
  case mqtt_socket_create:
    
    if(spwf01sa_open(&wifi_handle, (char*) m1_mqtt.hostname, m1_mqtt.port, m1_mqtt.protocol, (uint16_t*) &network.my_socket)<0){
      myprintf("\r\n [E]. Socket opening failed. Please check MQTT configuration. Trying reconnection.... \r\n");
      myprintf((char*)m1_mqtt.hostname);
      myprintf("\r\n");
      myprintf((char*)(&m1_mqtt.protocol));
      myprintf("\r\n");
      wifi_state=mqtt_socket_create;
      HAL_Delay(2000);
    }else{
      /* Initialize MQTT client structure */
      MQTTClient(&client,&network, 4000, MQTT_write_buf, sizeof(MQTT_write_buf), MQTT_read_buf, sizeof(MQTT_read_buf));
      wifi_state=mqtt_connect;
      
      myprintf("\r\n [D]. Created socket with MQTT broker. \r\n");
    }
    break;
    
  case mqtt_connect:     
    options.MQTTVersion = 3;
    options.clientID.cstring = (char*)m1_mqtt.clientid;
    options.username.cstring = (char*)m1_mqtt.username;
    options.password.cstring = (char*)m1_mqtt.password;
    
    if ( (ret_code = MQTTConnect(&client, &options)) != 0){
      myprintf("\r\n [E]. Client connection with MQTT broker failed with Error Code %d \r\n", (int)ret_code);
      switch (ret_code)
      {
      case C_REFUSED_PROT_V:
        myprintf("Connection Refused, unacceptable protocol version \r\n");
        break;
      case C_REFUSED_ID:
        myprintf("Connection Refused, identifier rejected \r\n");
        break;
      case C_REFUSED_SERVER:
        myprintf("Connection Refused, Server unavailable \r\n");
        break;
      case C_REFUSED_UID_PWD:
        myprintf("Connection Refused, bad user name or password \r\n");
        break;
      case C_REFUSED_UNAUTHORIZED:
        myprintf("Connection Refused, not authorized \r\n");
        break;
      }
      spwf01sa_close(&wifi_handle, network.my_socket);
      wifi_state = mqtt_socket_create;
      HAL_Delay(2000);
    }else {
      wifi_state = mqtt_sub;
    }
    HAL_Delay(1500);
    break;
    
  case mqtt_sub: 
    myprintf("\r\n [D] mqtt sub  \r\n");
    
    /* Subscribe to topic */
    myprintf("\r\n [D] Subscribing topic:  \r\n");
    myprintf((char *)m1_mqtt.sub_topic);
    
    if( MQTTSubscribe(&client, (char*)m1_mqtt.sub_topic, m1_mqtt.qos, messageArrived) < 0) {
      myprintf("\r\n [E]. Subscribe with MediumOne broker failed. Please check MQTT configuration. \r\n");
    } else {
      myprintf("\r\n [D] Subscribed to topic:  \r\n");
      myprintf((char *)m1_mqtt.sub_topic);
    }
    
    HAL_Delay(1500);
    wifi_state=mqtt_pub;
    break;
    
  case mqtt_pub:
    m1_generate_json(json_buffer, MQTT_BUFFER_SIZE);
    
    MQTT_msg.qos=QOS0;
    MQTT_msg.dup=0;
    MQTT_msg.retained=1;
    MQTT_msg.payload= (char *) json_buffer;
    MQTT_msg.payloadlen=strlen( (char *) json_buffer);
    
    /* Publish MQTT message */
    if ( MQTTPublish(&client,(char*)m1_mqtt.pub_topic,&MQTT_msg) < 0) {
      myprintf("\r\n [E]. Failed to publish data. Reconnecting to MQTT broker.... \r\n");
      wifi_state=mqtt_connect;
    }  else {
      myprintf("\r\n [D]. Sensor data are published to cloud \r\n");
      myprintf(MQTT_msg.payload);
      myprintf("\r\n");
      
      wifi_state=mqtt_yield;
      state_tick = HAL_GetTick();
      state_tick_aggr = HAL_GetTick();
    }
    break;
    
  case mqtt_pub_aggr:
    m1_generate_json_aggr(json_buffer, MQTT_BUFFER_SIZE);
    
    MQTT_msg.qos=QOS0;
    MQTT_msg.dup=0;
    MQTT_msg.retained=1;
    MQTT_msg.payload= (char *) json_buffer;
    MQTT_msg.payloadlen=strlen( (char *) json_buffer);
    
    /* Publish MQTT message */
    if ( MQTTPublish(&client,(char*)m1_mqtt.pub_topic,&MQTT_msg) < 0) {
      myprintf("\r\n [E]. Failed to publish data. Reconnecting to MQTT broker.... \r\n");
      wifi_state=mqtt_connect;
    }  else {
      myprintf("\r\n [D]. Sensor data are published to cloud \r\n");
      myprintf(MQTT_msg.payload);
      myprintf("\r\n");
      
      wifi_state=mqtt_yield;
    }
    break;
  case mqtt_yield:
    MQTTYield(&client, 50);
    
    /* New message if got threshold or timeout reached */
    if(m1_has_threshold() == 1 || (HAL_GetTick() - state_tick) > (m1_config.rate_s*1000))
    {
      state_tick = HAL_GetTick();
      wifi_state = mqtt_pub;
    }
    
    
    if((HAL_GetTick() - state_tick_aggr) > (m1_config.aggr_rate_s*1000))
    {
      state_tick_aggr = HAL_GetTick();
      wifi_state = mqtt_pub_aggr;
    }
    
    
    break;
    
  default:
    break;
  }     
  
}

void messageArrived(MessageData* md)
{
  MQTTMessage* message = md->message;
  uint16_t message_size = (uint16_t)message->payloadlen;
  *((char*)message->payload + message_size) = '\0';
  
  myprintf("\r\n [D]. MQTT payload received is: \r\n");
  myprintf((char*)message->payload);
  myprintf("\r\n");
  
  m1_process_messages(message->payload);
  
}

void HAL_GPIO_EXTI_Callback( uint16_t GPIO_Pin )
{
  if(GPIO_Pin == GPIO_PIN_13)
  {
    m1_inc_btn_pres();
  }
}
