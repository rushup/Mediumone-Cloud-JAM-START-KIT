#include "m1_mouserkit.h"
#include "parson.h"
#include "platform_usb.h"
#include <float.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "usart.h"
#include "app.h"
#include "main.h"
#include "platform_gnu.h"



#define M1_FLASH_ADD ((uint32_t)0x08060000)
#define M1_FLASH_ADD_L4 ((uint32_t)0x080FEE00)
#define M1_FLASH_SECTOR FLASH_SECTOR_7



typedef struct{
  int32_t min;
  int32_t max;
  int32_t avg;
  int32_t avg_n;
}t_aggr_int;

typedef struct{
  float min;
  float max;
  float avg;
  int32_t avg_n;
}t_aggr_float;

static const t_aggr_float aggr_float_std = {FLT_MAX,FLT_MIN,0,0};
static const t_aggr_int aggr_int_std = {INT32_MAX,INT32_MIN,0,0};

static bool has_config = false;
static t_aggr_float aggr_pressure = {FLT_MAX,FLT_MIN,0,0};
static t_aggr_float aggr_humidity = {FLT_MAX,FLT_MIN,0,0};
static t_aggr_float aggr_temperature = {FLT_MAX,FLT_MIN,0,0};
static t_aggr_int aggr_accx = {INT32_MAX,INT32_MIN,0,0};
static t_aggr_int aggr_accy = {INT32_MAX,INT32_MIN,0,0};
static t_aggr_int aggr_accz = {INT32_MAX,INT32_MIN,0,0};
static t_aggr_int aggr_gyrox = {INT32_MAX,INT32_MIN,0,0};
static t_aggr_int aggr_gyroy = {INT32_MAX,INT32_MIN,0,0};
static t_aggr_int aggr_gyroz = {INT32_MAX,INT32_MIN,0,0};
static t_aggr_int aggr_magx = {INT32_MAX,INT32_MIN,0,0};
static t_aggr_int aggr_magy = {INT32_MAX,INT32_MIN,0,0};
static t_aggr_int aggr_magz = {INT32_MAX,INT32_MIN,0,0};
static uint32_t btn_press_count = 0;
static uint32_t threshold_mask = 0;
static bool pending_threshold = false;

t_m1_config m1_config = {
  .rate_s = 20,
  .aggr_rate_s = 1*60*15,
  .heartbeat_active = false,
  .min_temp = FLT_MIN,
  .max_temp = FLT_MAX,
  .min_hum = FLT_MIN,
  .max_hum = FLT_MAX,
  .min_press = FLT_MIN,
  .max_press = FLT_MAX,
  .min_accx = INT32_MIN,
  .max_accx = INT32_MAX,
  .min_accy = INT32_MIN,
  .max_accy = INT32_MAX,
  .min_accz = INT32_MIN,
  .max_accz = INT32_MAX,
  .min_gyrox = INT32_MIN,
  .max_gyrox = INT32_MAX,
  .min_gyroy = INT32_MIN,
  .max_gyroy = INT32_MAX,
  .min_gyroz = INT32_MIN,
  .max_gyroz = INT32_MAX,
  .min_magx = INT32_MIN,
  .max_magx = INT32_MAX,
  .min_magy = INT32_MIN,
  .max_magy = INT32_MAX,
  .min_magz = INT32_MIN,
  .max_magz = INT32_MAX,
};

t_m1_net_config m1_net_config = {
  .magic = 0,
  .ssid = ""
};

t_m1_mqtt m1_mqtt = {
  .protocol = 's'
};

int8_t _get_json_number(JSON_Object* obj, const char* str, double* num)
{
  JSON_Value* val;
  val = json_object_get_value(obj, str);
  
  if(val != 0)
  {
    if(json_value_get_type(val) == JSONNumber)
    {
      //*num = val->value.number;
      *num = json_object_get_number(obj, str);
      return 0;
    }
  }
  return -1;
}

int8_t _get_json_bool(JSON_Object* obj, const char* str, bool* num)
{
  JSON_Value* val;
  val = json_object_get_value(obj, str);
  
  if(val != 0)
  {
    if(json_value_get_type(val) == JSONBoolean)
    {
      *num = json_object_get_boolean(obj, str);
      return 0;
    }
  }
  return -1;
}

#define DO_JSON_CONV_NUM(obj, str, tmp, type, conf) if(_get_json_number(obj, str, &tmp) == 0) conf = (type) tmp

#define DO_JSON_CONV_BOOL(obj, str, tmp, type, conf) if(_get_json_bool(obj, str, &tmp) == 0) conf = (type) tmp


void m1_generate_json(char* json, int size)
{
  JSON_Status status;
  JSON_Value *event_value = json_value_init_object();
  JSON_Object *event_object = json_value_get_object(event_value);
  JSON_Value *root_value = json_value_init_object();
  JSON_Object *root_object = json_value_get_object(root_value);
  
  status = json_object_set_number(root_object, "temperature", TEMPERATURE_Value);
  status = json_object_set_number(root_object, "humidity", HUMIDITY_Value);
  status = json_object_set_number(root_object, "pressure", PRESSURE_Value);
  status = json_object_set_number(root_object, "acc_x", ACC_Value.AXIS_X);
  status = json_object_set_number(root_object, "acc_y", ACC_Value.AXIS_Y);
  status = json_object_set_number(root_object, "acc_z", ACC_Value.AXIS_Z);
  status = json_object_set_number(root_object, "gyro_x", GYR_Value.AXIS_X);
  status = json_object_set_number(root_object, "gyro_y", GYR_Value.AXIS_Y);
  status = json_object_set_number(root_object, "gyro_z", GYR_Value.AXIS_Z);
  status = json_object_set_number(root_object, "mag_x", MAG_Value.AXIS_X);
  status = json_object_set_number(root_object, "mag_y", MAG_Value.AXIS_Y);
  status = json_object_set_number(root_object, "mag_z", MAG_Value.AXIS_Z);
  
  json_object_dotset_value(event_object, "event_data", root_value);
  
  json_serialize_to_buffer_nonpretty(event_value, json, size);
  
  json_value_free(event_value);
  
  if(status != JSONSuccess)
  {
	  myprintf("Json parse error \r\n");
  }
  
}


void m1_generate_json_aggr(char* json, int size)
{
  JSON_Status status;
  JSON_Value *event_value = json_value_init_object();
  JSON_Object *event_object = json_value_get_object(event_value);
  JSON_Value *root_value = json_value_init_object();
  JSON_Object *root_object = json_value_get_object(root_value);
  
  status = json_object_set_number(root_object, "min_temperature", aggr_temperature.min);
  status = json_object_set_number(root_object, "max_temperature", aggr_temperature.max);
  status = json_object_set_number(root_object, "avg_temperature", aggr_temperature.avg);
  
  status = json_object_set_number(root_object, "min_humidity", aggr_humidity.min);
  status = json_object_set_number(root_object, "max_humidity", aggr_humidity.max);
  status = json_object_set_number(root_object, "avg_humidity", aggr_humidity.avg);
  
  status = json_object_set_number(root_object, "min_pressure", aggr_pressure.min);
  status = json_object_set_number(root_object, "max_pressure", aggr_pressure.max);
  status = json_object_set_number(root_object, "avg_pressure", aggr_pressure.avg);
  
  status = json_object_set_number(root_object, "min_accx", aggr_accx.min);
  status = json_object_set_number(root_object, "max_accx", aggr_accx.max);
  status =  json_object_set_number(root_object, "avg_accx", aggr_accx.avg);
  
  status = json_object_set_number(root_object, "min_accy", aggr_accy.min);
  status = json_object_set_number(root_object, "max_accy", aggr_accy.max);
  status = json_object_set_number(root_object, "avg_accy", aggr_accy.avg);
  
  status = json_object_set_number(root_object, "min_accz", aggr_accz.min);
  status = json_object_set_number(root_object, "max_accz", aggr_accz.max);
  status = json_object_set_number(root_object, "avg_accz", aggr_accz.avg);
  
  status = json_object_set_number(root_object, "min_gyrox", aggr_gyrox.min);
  status = json_object_set_number(root_object, "max_gyrox", aggr_gyrox.max);
  status = json_object_set_number(root_object, "avg_gyrox", aggr_gyrox.avg);
  
  status = json_object_set_number(root_object, "min_gyroy", aggr_gyroy.min);
  status = json_object_set_number(root_object, "max_gyroy", aggr_gyroy.max);
  status = json_object_set_number(root_object, "avg_gyroy", aggr_gyroy.avg);
  
  status = json_object_set_number(root_object, "min_gyroz", aggr_gyroz.min);
  status = json_object_set_number(root_object, "max_gyroz", aggr_gyroz.max);
  status = json_object_set_number(root_object, "avg_gyroz", aggr_gyroz.avg);
  
  status = json_object_set_number(root_object, "min_magx", aggr_magx.min);
  status = json_object_set_number(root_object, "max_magx", aggr_magx.max);
  status = json_object_set_number(root_object, "avg_magx", aggr_magx.avg);
  
  status = json_object_set_number(root_object, "min_magy", aggr_magy.min);
  status = json_object_set_number(root_object, "max_magy", aggr_magy.max);
  status = json_object_set_number(root_object, "avg_magy", aggr_magy.avg);
  
  status = json_object_set_number(root_object, "min_magz", aggr_magz.min);
  status = json_object_set_number(root_object, "max_magz", aggr_magz.max);
  status = json_object_set_number(root_object, "avg_magz", aggr_magz.avg);
  
  status = json_object_set_number(root_object, "btn_count", btn_press_count);
  
  status = json_object_dotset_value(event_object, "event_data", root_value);
  
  json_serialize_to_buffer_nonpretty(event_value, json, size);
  
  json_value_free(event_value);
  //json_value_free(root_value);
  
  /* Reset aggregate */
  
  memcpy(&aggr_pressure, &aggr_float_std, sizeof(t_aggr_float));
  memcpy(&aggr_humidity, &aggr_float_std, sizeof(t_aggr_float));
  memcpy(&aggr_temperature, &aggr_float_std, sizeof(t_aggr_float));
  
  memcpy(&aggr_accx, &aggr_int_std, sizeof(t_aggr_int));
  memcpy(&aggr_accy, &aggr_int_std, sizeof(t_aggr_int));
  memcpy(&aggr_accz, &aggr_int_std, sizeof(t_aggr_int));
  
  memcpy(&aggr_gyrox, &aggr_int_std, sizeof(t_aggr_int));
  memcpy(&aggr_gyroy, &aggr_int_std, sizeof(t_aggr_int));
  memcpy(&aggr_gyroz, &aggr_int_std, sizeof(t_aggr_int));
  
  memcpy(&aggr_magx, &aggr_int_std, sizeof(t_aggr_int));
  memcpy(&aggr_magy, &aggr_int_std, sizeof(t_aggr_int));
  memcpy(&aggr_magz, &aggr_int_std, sizeof(t_aggr_int));
  
  btn_press_count = 0;
  
  if(status != JSONSuccess)
  {
	  myprintf("Json parse error \r\n");
  }
  
}

void m1_process_messages(char* json)
{
  JSON_Value* root = json_parse_string(json);
  JSON_Value* config = 0;
  JSON_Value* ledcmd = 0;
  JSON_Object* root_obj;
  char *str;
  double d_tmp;
  bool b_tmp;
  
  root_obj = json_value_get_object(root);
  
  if(root_obj != 0)
  {
    config = json_object_get_value(root_obj, "config");
    
    ledcmd = json_object_get_value(root_obj, "led");
    
  }
  
  if(ledcmd != 0)
  {
    str = (char*) json_value_get_string(ledcmd);
    
    if(strcmp(str, "off") == 0)
      HAL_GPIO_WritePin(LED1_OUTPUT_GPIO_Port, LED1_OUTPUT_Pin, GPIO_PIN_RESET);
    if(strcmp(str, "on") == 0)
      HAL_GPIO_WritePin(LED1_OUTPUT_GPIO_Port, LED1_OUTPUT_Pin, GPIO_PIN_SET);
    if(strcmp(str, "toggle") == 0)
      HAL_GPIO_TogglePin(LED1_OUTPUT_GPIO_Port, LED1_OUTPUT_Pin);
    
    
  }
  
  if(config != 0)
  {
    root_obj = json_value_get_object(config);
    if(root_obj != 0)
    {
      has_config = true;
      
      
      DO_JSON_CONV_NUM(root_obj, "rate_s", d_tmp, uint32_t, m1_config.rate_s);
      
      DO_JSON_CONV_NUM(root_obj, "aggr_rate_s", d_tmp, uint32_t, m1_config.aggr_rate_s);
      
      DO_JSON_CONV_BOOL(root_obj, "heartbeat_active", b_tmp, bool, m1_config.heartbeat_active);
      
      DO_JSON_CONV_NUM(root_obj, "min_temperature", d_tmp, float, m1_config.min_temp);
      
      DO_JSON_CONV_NUM(root_obj, "max_temperature", d_tmp, float, m1_config.max_temp);
      
      DO_JSON_CONV_NUM(root_obj, "min_pressure", d_tmp, float, m1_config.min_press);
      
      DO_JSON_CONV_NUM(root_obj, "max_pressure", d_tmp, float, m1_config.max_press);
      
      DO_JSON_CONV_NUM(root_obj, "min_humidity", d_tmp, float, m1_config.min_hum);
      
      DO_JSON_CONV_NUM(root_obj, "max_humidity", d_tmp, float, m1_config.max_hum);
      
      DO_JSON_CONV_NUM(root_obj, "max_accx", d_tmp, uint32_t, m1_config.max_accx);
      
      DO_JSON_CONV_NUM(root_obj, "min_accx", d_tmp, uint32_t, m1_config.min_accx);
      
      DO_JSON_CONV_NUM(root_obj, "max_accy", d_tmp, uint32_t, m1_config.max_accy);
      
      DO_JSON_CONV_NUM(root_obj, "min_accy", d_tmp, uint32_t, m1_config.min_accy);
      
      DO_JSON_CONV_NUM(root_obj, "max_accz", d_tmp, uint32_t, m1_config.max_accz);
      
      DO_JSON_CONV_NUM(root_obj, "min_accz", d_tmp, uint32_t, m1_config.min_accz);
      
      DO_JSON_CONV_NUM(root_obj, "max_gyrox", d_tmp, uint32_t, m1_config.max_gyrox);
      
      DO_JSON_CONV_NUM(root_obj, "min_gyrox", d_tmp, uint32_t, m1_config.min_gyrox);
      
      DO_JSON_CONV_NUM(root_obj, "min_gyroy", d_tmp, uint32_t, m1_config.min_gyroy);
      
      DO_JSON_CONV_NUM(root_obj, "max_gyroy", d_tmp, uint32_t, m1_config.max_gyroy);
      
      DO_JSON_CONV_NUM(root_obj, "min_gyroz", d_tmp, uint32_t, m1_config.min_gyroz);
      
      DO_JSON_CONV_NUM(root_obj, "max_gyroz", d_tmp, uint32_t, m1_config.max_gyroz);
      
      DO_JSON_CONV_NUM(root_obj, "max_magx", d_tmp, uint32_t, m1_config.max_magx);
      
      DO_JSON_CONV_NUM(root_obj, "min_magx", d_tmp, uint32_t, m1_config.min_magx);
      
      DO_JSON_CONV_NUM(root_obj, "max_magy", d_tmp, uint32_t, m1_config.max_magy);
      
      DO_JSON_CONV_NUM(root_obj, "min_magy", d_tmp, uint32_t, m1_config.min_magy);
      
      DO_JSON_CONV_NUM(root_obj, "max_magz", d_tmp, uint32_t, m1_config.max_magz);
      
      DO_JSON_CONV_NUM(root_obj, "max_magz", d_tmp, uint32_t, m1_config.max_magz);
    }
    
  }
  
  if(root != 0)
  {
    json_value_free(root);
  }
  
  
}

void m1_poll_sensors()
{
  //TEMP
  if(aggr_temperature.min > TEMPERATURE_Value)
    aggr_temperature.min = TEMPERATURE_Value;
  
  if(aggr_temperature.max < TEMPERATURE_Value)
    aggr_temperature.max = TEMPERATURE_Value;
  
  aggr_temperature.avg = (aggr_temperature.avg * aggr_temperature.avg_n + TEMPERATURE_Value) / (aggr_temperature.avg_n + 1);
  aggr_temperature.avg_n++;

  
  //HUM
  if(aggr_humidity.min > HUMIDITY_Value)
    aggr_humidity.min = HUMIDITY_Value;
  
  if(aggr_humidity.max < HUMIDITY_Value)
    aggr_humidity.max = HUMIDITY_Value;
  
  aggr_humidity.avg = (aggr_humidity.avg * aggr_humidity.avg_n + HUMIDITY_Value) / (aggr_humidity.avg_n + 1);
  aggr_humidity.avg_n++;

  //PRESS
  
  if(aggr_pressure.min > PRESSURE_Value)
    aggr_pressure.min = PRESSURE_Value;
  
  if(aggr_pressure.max < PRESSURE_Value)
    aggr_pressure.max = PRESSURE_Value;
  
  aggr_pressure.avg = (aggr_pressure.avg * aggr_pressure.avg_n + PRESSURE_Value) / (aggr_pressure.avg_n + 1);
  aggr_pressure.avg_n++;

  //ACCX
  
  if(aggr_accx.min > ACC_Value.AXIS_X)
    aggr_accx.min = ACC_Value.AXIS_X;
  
  if(aggr_accx.max < ACC_Value.AXIS_X)
    aggr_accx.max = ACC_Value.AXIS_X;
  
  aggr_accx.avg = (aggr_accx.avg * aggr_accx.avg_n + ACC_Value.AXIS_X) / (aggr_accx.avg_n + 1);
  aggr_accx.avg_n++;

  //ACCY
  
  if(aggr_accy.min > ACC_Value.AXIS_Y)
    aggr_accy.min = ACC_Value.AXIS_Y;
  
  if(aggr_accy.max < ACC_Value.AXIS_Y)
    aggr_accy.max = ACC_Value.AXIS_Y;
  
  
  aggr_accy.avg = (aggr_accy.avg * aggr_accy.avg_n + ACC_Value.AXIS_Y) / (aggr_accy.avg_n + 1);
  aggr_accy.avg_n++;
    

  //ACCZ
  
  if(aggr_accz.min > ACC_Value.AXIS_Z)
    aggr_accz.min = ACC_Value.AXIS_Z;
  
  if(aggr_accz.max < ACC_Value.AXIS_Z)
    aggr_accz.max = ACC_Value.AXIS_Z;
  
  aggr_accz.avg = (aggr_accz.avg * aggr_accz.avg_n + ACC_Value.AXIS_Z) / (aggr_accz.avg_n + 1);
  aggr_accz.avg_n++;

  //GYROX
  
  if(aggr_gyrox.min > GYR_Value.AXIS_X)
    aggr_gyrox.min = GYR_Value.AXIS_X;
  
  if(aggr_gyrox.max < GYR_Value.AXIS_X)
    aggr_gyrox.max = GYR_Value.AXIS_X;
  
  aggr_gyrox.avg = (aggr_gyrox.avg * aggr_gyrox.avg_n + GYR_Value.AXIS_X) / (aggr_gyrox.avg_n + 1);
  aggr_gyrox.avg_n++;

  //GYROY
  
  if(aggr_gyroy.min > GYR_Value.AXIS_Y)
    aggr_gyroy.min = GYR_Value.AXIS_Y;
  
  if(aggr_gyroy.max < GYR_Value.AXIS_Y)
    aggr_gyroy.max = GYR_Value.AXIS_Y;
  
  aggr_gyroy.avg = (aggr_gyroy.avg * aggr_gyroy.avg_n + GYR_Value.AXIS_Y) / (aggr_gyrox.avg_n + 1);
  aggr_gyroy.avg_n++;

  //GYROZ
  
  if(aggr_gyroz.min > GYR_Value.AXIS_Z)
    aggr_gyroz.min = GYR_Value.AXIS_Z;
  
  if(aggr_gyroz.max < GYR_Value.AXIS_Z)
    aggr_gyroz.max = GYR_Value.AXIS_Z;
  
  aggr_gyroz.avg = (aggr_gyroz.avg * aggr_gyroz.avg_n + GYR_Value.AXIS_Z) / (aggr_gyrox.avg_n + 1);
  aggr_gyroz.avg_n++;
  //MAGX
  
  if(aggr_magx.min > MAG_Value.AXIS_X)
    aggr_magx.min = MAG_Value.AXIS_X;
  
  if(aggr_magx.max < MAG_Value.AXIS_X)
    aggr_magx.max = MAG_Value.AXIS_X;
  
  aggr_magx.avg = (aggr_magx.avg * aggr_magx.avg_n + MAG_Value.AXIS_X) / (aggr_magx.avg_n + 1);
  aggr_magx.avg_n++;
 
  //MAGY
  
  if(aggr_magy.min > MAG_Value.AXIS_Y)
    aggr_magy.min = MAG_Value.AXIS_Y;
  
  if(aggr_magy.max < MAG_Value.AXIS_Y)
    aggr_magy.max = GYR_Value.AXIS_Y;
  
  aggr_magy.avg = (aggr_magy.avg * aggr_magy.avg_n + MAG_Value.AXIS_Y) / (aggr_magy.avg_n + 1);
  aggr_magy.avg_n++;

  //MAGZ
  
  if(aggr_magz.min > MAG_Value.AXIS_Z)
    aggr_magz.min = MAG_Value.AXIS_Z;
  
  if(aggr_magz.max < MAG_Value.AXIS_Z)
    aggr_magz.max = GYR_Value.AXIS_Z;
  
  aggr_magz.avg = (aggr_magz.avg * aggr_magz.avg_n + MAG_Value.AXIS_Z) / (aggr_magz.avg_n + 1);
  aggr_magz.avg_n++;
  
  /* Check threshold */
  
  uint32_t l_mask = 0;
  static bool first_poll = false;
  
  if(TEMPERATURE_Value >= m1_config.max_temp)
    l_mask |= 0x01;
  else
    l_mask &= ~0x01;
  
  if(TEMPERATURE_Value <= m1_config.min_temp)
    l_mask |= 0x02;
  else
    l_mask &= ~0x02;

  if(HUMIDITY_Value >= m1_config.max_hum)
    l_mask |= 0x02;
  else
    l_mask &= ~0x02;
  
  if(HUMIDITY_Value <= m1_config.min_hum)
    l_mask |= 0x04;
  else
    l_mask &= ~0x04;
  
  if(PRESSURE_Value >= m1_config.max_press)
    l_mask |= 0x08;
  else
    l_mask &= ~0x08;
  
  if(PRESSURE_Value <= m1_config.min_press)
    l_mask |= 0x10;
  else
    l_mask &= ~0x10;
  
  if(ACC_Value.AXIS_X > m1_config.max_accx)
    l_mask |= 0x20;
  else
    l_mask &= ~0x20;
  
  if(ACC_Value.AXIS_X < m1_config.min_accx)
    l_mask |= 0x40;
  else
    l_mask &= ~0x40;
  
  if(ACC_Value.AXIS_Y > m1_config.max_accy)
    l_mask |= 0x80;
  else
    l_mask &= ~0x80;
  
  if(ACC_Value.AXIS_Y < m1_config.min_accy)
    l_mask |= 0x100;
  else
    l_mask &= ~0x100;
  
  if(ACC_Value.AXIS_Z > m1_config.max_accz)
    l_mask |= 0x100;
  else
    l_mask &= ~0x100;
  
  if(ACC_Value.AXIS_Z < m1_config.min_accz)
    l_mask |= 0x200;
  else
    l_mask &= ~0x200;
  
  if(GYR_Value.AXIS_X > m1_config.max_gyrox)
    l_mask |= 0x800;
  else
    l_mask &= ~0x800;
  
  if(GYR_Value.AXIS_X < m1_config.min_gyrox)
    l_mask |= 0x400;
  else
    l_mask &= ~0x400;
  
  if(GYR_Value.AXIS_Y > m1_config.max_gyroy)
    l_mask |= 0x1000;
  else
    l_mask &= ~0x1000;
  
  if(GYR_Value.AXIS_Y < m1_config.min_gyroy)
    l_mask |= 0x2000;
  else
    l_mask &= ~0x2000;
   
  if(GYR_Value.AXIS_Z > m1_config.max_gyroz)
    l_mask |= 0x1000;
  else
    l_mask &= ~0x1000;
  
  if(GYR_Value.AXIS_Z < m1_config.min_gyroz)
    l_mask |= 0x2000;
  else
    l_mask &= ~0x2000;
    
  if(MAG_Value.AXIS_X > m1_config.max_magx)
    l_mask |= 0x4000;
  else
    l_mask &= ~0x4000;
  
  if(MAG_Value.AXIS_X < m1_config.min_magx)
    l_mask |= 0x8000;
  else
    l_mask &= ~0x8000;

    
  if(MAG_Value.AXIS_Y < m1_config.min_magy)
    l_mask |= 0x10000;
  else
    l_mask &= ~0x10000;
  
  if(MAG_Value.AXIS_Y > m1_config.max_magy)
    l_mask |= 0x20000;
  else
    l_mask &= ~0x20000;
  
  
  if(MAG_Value.AXIS_Z < m1_config.min_magz)
    l_mask |= 0x40000;
  else
    l_mask &= ~0x40000;
  
  if(MAG_Value.AXIS_Z > m1_config.max_magz)
    l_mask |= 0x80000;
  else
    l_mask &= ~0x80000;
  
  if(first_poll == false)
  {
    first_poll = true;
    threshold_mask = l_mask;
    
    pending_threshold = false;
  }
  else
  {
    if(threshold_mask != l_mask)
    {
      threshold_mask = l_mask;
      pending_threshold = true;
    }
  }
  
}

bool m1_has_threshold()
{
  if(pending_threshold == true)
  {
    pending_threshold = false;
    return true;
  }
  else
    return false;
}

void m1_inc_btn_pres()
{
  btn_press_count++;
}

static void _query_param(const char* ask_str, char* fill_buff, int max_fill_buf_size)
{
  uint16_t console_count=0;
  bool end = false;
  
  myprintf("\r\n%s",ask_str);

  console_count=0;
  
  usb_data_buffer_clear();
  
  for(;;)
  {
    
    console_count = strlen(usb_data_buffer);
    
    if(console_count > 0)
    {
      while(1){
        if(console_count > 0 && (usb_data_buffer[console_count - 1] == '\r' || usb_data_buffer[console_count - 1] == '\n'))
        {
          usb_data_buffer[console_count - 1] = 0;
          console_count--;
          end = true;
        }
        else
          break;
      }
    }
    
    if(end == true)
      break;
  }
  
  if(console_count > 0)
  {
    strncpy(fill_buff, usb_data_buffer, max_fill_buf_size);
  }
  else
  {
	  myprintf("Skipping\r\n");
  }
  
  myprintf("\r\n");
}

static bool _read_param_from_flash()
{
  asm("nop");
#ifdef USE_STM32L4XX_NUCLEO
  t_m1_net_config* flash_conf = (t_m1_net_config*) M1_FLASH_ADD_L4;
#else
  t_m1_net_config* flash_conf = (t_m1_net_config*) M1_FLASH_ADD;
  
#endif
  if(flash_conf->magic == 0xAAAAAAAA)
  {
    memcpy(&m1_net_config, flash_conf, M1_NET_CONFIG_SIZE);
    return true;
  }
  else
    return false;
}

#ifdef USE_STM32L4XX_NUCLEO
/**
  * @brief  Gets the page of a given address
  * @param  Addr: Address of the FLASH Memory
  * @retval The page of a given address
  */
static uint32_t _get_page(uint32_t Addr)
{
  uint32_t page = 0;
  
  if (Addr < (FLASH_BASE + FLASH_BANK_SIZE))
  {
    /* Bank 1 */
    page = (Addr - FLASH_BASE) / FLASH_PAGE_SIZE;
  }
  else
  {
    /* Bank 2 */
    page = (Addr - (FLASH_BASE + FLASH_BANK_SIZE)) / FLASH_PAGE_SIZE;
  }
  
  return page;
}

/**
  * @brief  Gets the bank of a given address
  * @param  Addr: Address of the FLASH Memory
  * @retval The bank of a given address
  */
static uint32_t _get_bank(uint32_t Addr)
{
  uint32_t bank = 0;
  
  if (READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_FB_MODE) == 0)
  {
  	/* No Bank swap */
    if (Addr < (FLASH_BASE + FLASH_BANK_SIZE))
    {
      bank = FLASH_BANK_1;
    }
    else
    {
      bank = FLASH_BANK_2;
    }
  }
  else
  {
  	/* Bank swap */
    if (Addr < (FLASH_BASE + FLASH_BANK_SIZE))
    {
      bank = FLASH_BANK_2;
    }
    else
    {
      bank = FLASH_BANK_1;
    }
  }
  
  return bank;
}

#endif

static void _save_param_to_flash()
{
#ifdef USE_STM32L4XX_NUCLEO
  uint32_t Address = M1_FLASH_ADD_L4;
  int32_t WriteIndex;
  FLASH_EraseInitTypeDef EraseInitStruct;
  uint32_t FirstPage = 0, NbOfPages = 0, BankNumber = 0;
  uint32_t PAGEError = 0;
  __IO uint32_t data32 = 0 , MemoryProgramStatus = 0;
  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();
  
  /* Erase the user Flash area
  (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/
  
  /* Clear OPTVERR bit set on virgin samples */

  /* Get the 1st page to erase */
  FirstPage = _get_page(M1_FLASH_ADD_L4);
  /* Get the number of pages to erase from 1st page */
  NbOfPages = 2;
  /* Get the bank */
  BankNumber = _get_bank(M1_FLASH_ADD_L4);
  /* Fill EraseInit structure*/
  EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
  EraseInitStruct.Banks       = BankNumber;
  EraseInitStruct.Page        = FirstPage;
  EraseInitStruct.NbPages     = NbOfPages;
  

  /* Note: If an erase operation in Flash memory also concerns data in the data or instruction cache,
     you have to make sure that these data are rewritten before they are accessed during code
     execution. If this cannot be done safely, it is recommended to flush the caches by setting the
     DCRST and ICRST bits in the FLASH_CR register. */
  if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
  {
    asm("nop");
  }
  
  for(WriteIndex=0; WriteIndex < (M1_NET_CONFIG_SIZE / 8);WriteIndex++){
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, Address, ((uint64_t*)&m1_net_config)[WriteIndex]) == HAL_OK){
      Address = Address + 8;
    } else {
    	myprintf("Error while writing in FLASH\r\n");
      break;
    }
  }
  HAL_FLASH_Lock();
#else
  uint32_t Address = M1_FLASH_ADD;
  int32_t WriteIndex;
  FLASH_EraseInitTypeDef EraseInitStruct;
  uint32_t SectorError = 0;
  
  EraseInitStruct.TypeErase = TYPEERASE_SECTORS;
  EraseInitStruct.VoltageRange = VOLTAGE_RANGE_3;
  EraseInitStruct.Sector = M1_FLASH_SECTOR;
  EraseInitStruct.NbSectors = 1;
  
  HAL_FLASH_Unlock();
  
  if(HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK)
  {
    HAL_FLASH_Lock();
    return;
  }
  
  for(WriteIndex=0; WriteIndex < (M1_NET_CONFIG_SIZE / 4);WriteIndex++){
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address, ((uint32_t*)&m1_net_config)[WriteIndex]) == HAL_OK){
      Address = Address + 4;
    } else {
    	myprintf("Error while writing in FLASH\r\n");
      break;
    }
  }
  
  HAL_FLASH_Lock();
#endif

  
  

}

void m1_ask_net_parameters()
{
  char console_input[1];
  
  myprintf("Press enter to skip parameter set \r\n");
  
  m1_net_config.magic = 0xAAAAAAAA;
  
  _query_param("Enter the SSID: ", m1_net_config.ssid, sizeof(m1_net_config.ssid));
  
  _query_param("Enter the password: ", m1_net_config.password, sizeof(m1_net_config.password));
  
  _query_param("Enter the authentication mode(0:Open, 1:WEP, 2:WPA2/WPA2-Personal): ", console_input, 1);
  
  switch(console_input[0])
  {
  case '0':
    strcpy(m1_net_config.auth, "NONE");
    break;
  case '1':
    strcpy(m1_net_config.auth, "WEP");
    break;
  case '2':
    strcpy(m1_net_config.auth, "WPA2");
    break;
  }
  
  _query_param("Enter MQTT Project ID: ", m1_net_config.m1_project_mqtt_id, sizeof(m1_net_config.m1_project_mqtt_id));
  
  _query_param("Enter User ID: ", m1_net_config.m1_user_id, sizeof(m1_net_config.m1_user_id));
  
  _query_param("Enter User Password: ", m1_net_config.m1_user_password, sizeof(m1_net_config.m1_user_password));
  
  _query_param("Enter User APIKEY: ", m1_net_config.m1_api_key, sizeof(m1_net_config.m1_api_key));
  
  _query_param("Enter Device Name (Default: CLOUDJAM): ", m1_net_config.m1_device, sizeof(m1_net_config.m1_device));
  if(strlen( m1_net_config.m1_device) < 2)
    strcpy( m1_net_config.m1_device, "CLOUDJAM");
  
  if(strlen(m1_net_config.ssid) > 0 && strlen(m1_net_config.password) > 0)
    _save_param_to_flash();
}

void m1_dump_parameter()
{
	myprintf("\r\nCurrent configuration: \r\n");
  
	myprintf("SSID: %s \r\n", m1_net_config.ssid);
  
	myprintf("Password: %s \r\n", m1_net_config.password);
  
	myprintf("Authentication mode: %s \r\n", m1_net_config.auth);
  
	myprintf("MQTT Project ID: %s \r\n", m1_net_config.m1_project_mqtt_id);
  
	myprintf("User ID: %s \r\n", m1_net_config.m1_user_id);
  
	myprintf("User Password: %s \r\n", m1_net_config.m1_user_password);
  
	myprintf("User APIKEY: %s \r\n", m1_net_config.m1_api_key);
  
	myprintf("Device name: %s \r\n", m1_net_config.m1_device);
  
	myprintf("\r\n");
}

void m1_init()
{
  if(_read_param_from_flash() == false)
  {
    memset(&m1_net_config,0,sizeof(t_m1_net_config));
    myprintf("No configuration found\r\n");
    
    m1_ask_net_parameters();
    m1_dump_parameter();
  }
  else
  {
    m1_dump_parameter();
    
    myprintf("\r\nKeep pressed user button to set WiFi and Medium One parameters...\r\n");

    HAL_Delay(3000);
    
    if(HAL_GPIO_ReadPin(BTN_INPUT_GPIO_Port, BTN_INPUT_Pin) == GPIO_PIN_RESET) {
      m1_ask_net_parameters();
      m1_dump_parameter();
    }
    
    btn_press_count = 0;
  }
}

void m1_config_mqtt( uint8_t *macadd )
{
  strcpy((char*)m1_mqtt.pub_topic, "0/");
  strcat((char*)m1_mqtt.pub_topic, m1_net_config.m1_project_mqtt_id);
  strcat((char*)m1_mqtt.pub_topic,"/");
  strcat((char*)m1_mqtt.pub_topic, m1_net_config.m1_user_id);
  strcat((char*)m1_mqtt.pub_topic,"/");
  strcat((char*)m1_mqtt.pub_topic, m1_net_config.m1_device);
  strcat((char*)m1_mqtt.pub_topic,"/");
  
  strcpy((char*)m1_mqtt.sub_topic, "1/");
  strcat((char*)m1_mqtt.sub_topic, m1_net_config.m1_project_mqtt_id);
  strcat((char*)m1_mqtt.sub_topic,"/");
  strcat((char*)m1_mqtt.sub_topic, m1_net_config.m1_user_id);
  strcat((char*)m1_mqtt.sub_topic,"/");
  strcat((char*)m1_mqtt.sub_topic, m1_net_config.m1_device);
  strcat((char*)m1_mqtt.sub_topic,"/#");
  
  m1_mqtt.qos = QOS0;
  
  strcpy((char*)m1_mqtt.username, m1_net_config.m1_project_mqtt_id);
  strcat((char*)m1_mqtt.username,"/");
  strcat((char*)m1_mqtt.username,m1_net_config.m1_user_id);
  
  strcpy((char*)m1_mqtt.password, m1_net_config.m1_api_key);
  strcat((char*)m1_mqtt.password,"/");
  strcat((char*)m1_mqtt.password,m1_net_config.m1_user_password);
  
  strcpy((char*)m1_mqtt.hostname,"mqtt.mediumone.com");
  strcpy((char*)m1_mqtt.device_type,"cloud_jam");
  strcpy((char*)m1_mqtt.org_id,"m1u8i");
  strcpy((char *)m1_mqtt.clientid, "d:");
  strcat((char *)m1_mqtt.clientid, (char *)m1_mqtt.org_id);
  strcat((char *)m1_mqtt.clientid,":");
  strcat((char *)m1_mqtt.clientid,(char *)m1_mqtt.device_type);
  strcat((char *)m1_mqtt.clientid,":");
  strcat((char*)m1_mqtt.clientid,(char *)macadd);
  
  m1_mqtt.port = 61620 ; //TLS
  m1_mqtt.protocol = 's'; // TLS no certificates
  
}
