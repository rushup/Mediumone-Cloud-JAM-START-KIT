#include "spwf01sa.h"
#include <atbox/at/at.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define SEND_BUF_SIZE 200

/* AT commands index code */
#define AT_WIND 4

/* Process commands */
#define CMD_IDLE 0
#define CMD_SCAN 1
#define CMD_SOCKET_OPEN 2
#define CMD_RECEIVE_DATA 3
#define CMD_QUERY_SOCKET_DATALEN 4
#define CMD_GET_CONFIG 5
#define CMD_GET_STATUS 6
#define CMD_GENERIC 200

/* AT Command Matches */
const static t_at_match res_match[] = {
  AT_ADD_MATCH("OK", AT_OK)
  AT_ADD_MATCH("FAIL", AT_FAIL)
  AT_ADD_MATCH("ERROR", AT_ERROR)
  AT_ADD_MATCH("ERROR:", AT_ERROR)
  AT_ADD_MATCH("+WIND:", AT_WIND)
  {0,0}
};

/* Prototypes */
static void usart_fetch_byte(void* handle, char byte);

static uint8_t _parse_ap(char* buff, t_wifi_ap* ap)
{
  char* t;
  char * p_end;
  int i;
  long int ti;

  t = strstr(buff, "BSS ") + strlen("BSS ");

  if(t != 0)
  {
    for(i = 0; i < 6; i++)
    {
      ti = strtol (t, &p_end, 16);
      if((*p_end == ':' && i < 5) || (*p_end == ' ' && i == 5))
      {
        t = p_end + 1;
        ap->mac[i] = ti;
      }
      else
        return 0;
    }
  }
  else
    return 0;

  t = strstr(buff, "CHAN: ") + strlen("CHAN: ");
  if(t != 0)
  {
    ti = strtol (t, &p_end, 10);
    if(*p_end == ' ')
    {
      ap->channel = ti;
    }
    else
      return 0;
  }
  else
    return 0;

  t = strstr(buff, "RSSI: ") + strlen("RSSI: ");
  if(t != 0)
  {
    ti = strtol (t, &p_end, 10);
    if(*p_end == ' ')
    {
      ap->rssi = ti;
    }
    else
      return 0;
  }
  else
    return 0;

  t = strstr(buff, "SSID: '") + strlen("SSID: '");
  if(t != 0)
  {
    for(i=0; i < 33; i++)
    {
      if(*(t+i) == '\'')
      {
        ap->ssid[i] = 0;

        ap->sec = WIFI_SEC_OPEN;

        if(strstr(t+i," WPA") != NULL)
          ap->sec = WIFI_SEC_WPA;
        if(strstr(t+i," WPA2") != NULL)
          ap->sec = WIFI_SEC_WPA2;
        if(ap->sec == WIFI_SEC_OPEN && strstr(t+i," WEP") != NULL)
          ap->sec = WIFI_SEC_WEP;

        return 1;
      }
      ap->ssid[i] = *(t + i);
    }
  }
  else
    return 0;

  return 0;
}

static void usart_fetch_byte(void* handle, char byte)
{
  uint8_t scanris;
  uint16_t at_ris;
  uint8_t event_code;
  t_wifi_spwf01sa_handle* h = (t_wifi_spwf01sa_handle*) handle;
  
  if(h->priv.processing_cmd == CMD_RECEIVE_DATA)
  {
    h->priv.process_buffer_out[h->priv.process_buffer_out_index++] = byte;
    
    if(h->priv.process_buffer_out_index >= h->priv.process_buffer_out_recv_len)
    {
      /* Go waiting OK */
      h->priv.process_buffer_out_index = 0;
      h->priv.processing_cmd = CMD_GENERIC;
    }
    
    return;
    
  }
  
  at_ris = at_process_match(&h->priv.at_handle, byte, res_match, sizeof(res_match) / sizeof(t_at_match));
  
  if(at_ris != 0)
  {
    switch(h->priv.processing_cmd)
    {
    case CMD_SCAN:
      if(at_ris == AT_UNKNOWN)
      {
        scanris = _parse_ap(at_get_cmd_cache(&h->priv.at_handle), (t_wifi_ap*)h->priv.cb_buffer);
        
        if(scanris == 1)
        {

          ((t_wifi_ap*)h->priv.cb_buffer)->active = true;
          
          h->callbacks.event_callback(WIFI_FOUND_AP, h->priv.cb_buffer, sizeof(t_wifi_ap));
          
          if(strncmp(((t_wifi_ap*)h->priv.cb_buffer)->ssid, ((t_wifi_ap*)h->priv.process_buffer_in)->ssid, MAX_SSID_LEN) == 0)
          {
            memcpy(h->priv.process_buffer_out, h->priv.cb_buffer, sizeof(t_wifi_ap));
          }

        }
        
        memset(h->priv.cb_buffer, 0, sizeof(h->priv.cb_buffer));

      }
      break;
    case CMD_SOCKET_OPEN:
      if(at_ris == AT_UNKNOWN)
      {
        /* match must be at beginning */
        if(strstr(at_get_cmd_cache(&h->priv.at_handle), " ID: ") == at_get_cmd_cache(&h->priv.at_handle))
        {
          *((uint16_t*)h->priv.process_buffer_out) = atoi(at_get_cmd_cache(&h->priv.at_handle) + strlen(" ID: "));
        }
      }
      break;
    case CMD_QUERY_SOCKET_DATALEN:
      if(at_ris == AT_UNKNOWN)
      {
        if(strstr(at_get_cmd_cache(&h->priv.at_handle), " DATALEN: ") == at_get_cmd_cache(&h->priv.at_handle))
        {
          *((uint16_t*)h->priv.process_buffer_out) = atoi(at_get_cmd_cache(&h->priv.at_handle) + strlen(" DATALEN: "));
        }
      }
      break;
    case CMD_GET_CONFIG:
    case CMD_GET_STATUS:
      if(at_ris == AT_UNKNOWN)
      {
        if(strstr(at_get_cmd_cache(&h->priv.at_handle), h->priv.process_buffer_in) == at_get_cmd_cache(&h->priv.at_handle))
        {
          strncpy(h->priv.process_buffer_out, at_get_cmd_cache(&h->priv.at_handle) + strlen(h->priv.process_buffer_in), 100);
          if(strlen(h->priv.process_buffer_out) > 2)
          {
            h->priv.process_buffer_out[strlen(h->priv.process_buffer_out) - 1] = 0;
            h->priv.process_buffer_out[strlen(h->priv.process_buffer_out) - 1] = 0;
          }
        }
      }
      break;
    }
    
    switch(at_ris)
    {
    case AT_OK:
      h->priv.processing_cmd_result = WIFI_OK;
      break;
    case AT_ERROR:
      h->priv.processing_cmd_result = WIFI_ERROR;
      break;
    case AT_FAIL:
      h->priv.processing_cmd_result = WIFI_ERROR;
      break;
    case AT_WIND:
      sscanf(at_get_cmd_cache(&h->priv.at_handle), "+WIND:%hhu:", &event_code);
      switch(event_code)
      {
        /* Power Up */
        case 0: 
          h->priv.sta_connected = false;
          h->priv.poweron = true;
          break;
        /* HW Ready */
        case 32: 
          h->callbacks.event_callback(WIFI_READY, 0, 0);
          h->priv.sta_connected = false;
          h->priv.ready = true;
          break;
        /* Wifi Up*/
        case 24:
          h->callbacks.event_callback(WIFI_CONNECTED, 0, 0);

          sscanf(at_get_cmd_cache(&h->priv.at_handle), "+WIND:24:WiFi Up:%hhd.%hhd.%hhd.%hhd", 
                 &h->priv.ip[0],
                 &h->priv.ip[1],
                 &h->priv.ip[2],
                 &h->priv.ip[3]);
          h->callbacks.event_callback(WIFI_GOT_IP, (char*) h->priv.ip, 4);
          
          h->priv.sta_connected = true;
          break;
        /* Wifi Connection down */
        case 41:
          h->callbacks.event_callback(WIFI_DISCONNECTED, 0, 0);
          h->priv.sta_connected = false;
          break;
      }
      break;
    }
    
    at_clear_cache(&h->priv.at_handle);

    /* Trigger event */
    if(h->callbacks.os_quit_sleep != 0)
      h->callbacks.os_quit_sleep();
  }
  

  

}

static bool wait_var_change( t_wifi_spwf01sa_handle* h, char* ptr, char* ptr_check, uint8_t ptr_size, bool is_equal, uint32_t timeout)
{
  uint32_t tick = h->callbacks.get_tick_ms();
  
  while( (h->callbacks.get_tick_ms() - tick) < timeout)
  {
    
    if(is_equal)
    {
      if(memcmp(ptr, ptr_check, ptr_size) == 0)
        return true;
    }
    else
    {
      if(memcmp(ptr, ptr_check, ptr_size) != 0)
        return true;
    }
    
    if(h->callbacks.os_quit_sleep != 0)
    {
      /* In RTOS leave other task breath */
      h->callbacks.delay_ms(1000);
    }

  }
  
  return false;
}

static void wifi_clear_flags(t_wifi_spwf01sa_handle* h)
{
  h->priv.sta_connected = 0;
  h->priv.ready = 0;
  h->priv.poweron = 0;
}

e_wifi_ris spwf01sa_init(t_wifi_spwf01sa_handle* h, bool factory_reset)
{
  const bool c_true = true;
  bool wr = false;

  h->callbacks.gpio_write_reset(0);
  h->callbacks.delay_ms(100);
  h->callbacks.gpio_write_reset(1);
  
  if(factory_reset == true)
  {
    h->callbacks.delay_ms(1000);
    spwf01sa_factory_reset(h);
    h->callbacks.gpio_write_reset(0);
    h->callbacks.delay_ms(100);
    h->callbacks.gpio_write_reset(1);
  }
  
  wr = wait_var_change(h, (char*) &h->priv.poweron, (char*) &c_true, sizeof(bool), true, 5000);
  
  return (wr == true)?WIFI_OK:WIFI_TIMEOUT;
}

e_wifi_ris spwf01sa_wait_ready(t_wifi_spwf01sa_handle* h)
{
  const bool c_true = true;
  bool wr = false;
  
  wr = wait_var_change(h, (char*) &h->priv.ready, (char*) &c_true, sizeof(bool), true, 10000);
  
  return (wr == true)?WIFI_OK:WIFI_TIMEOUT;
}

e_wifi_ris spwf01sa_scan(t_wifi_spwf01sa_handle* h, const char* ssid, t_wifi_ap* r_ap)
{
  e_wifi_ris ris;
  const e_wifi_ris timeout = WIFI_TIMEOUT;
  t_wifi_ap* desired_ap;
  uint8_t retry_count = 0;
  char send_buf[SEND_BUF_SIZE];
  
retry:
  memset(h->priv.process_buffer_out, 0, sizeof(h->priv.process_buffer_out));
  memset(h->priv.process_buffer_in, 0, sizeof(h->priv.process_buffer_in));
  desired_ap = (t_wifi_ap*) h->priv.process_buffer_in;
  
  memset(send_buf, 0, SEND_BUF_SIZE);
  
  strncpy(desired_ap->ssid, ssid, MAX_SSID_LEN);
  
  snprintf(send_buf, SEND_BUF_SIZE, "AT+S.SCAN\r\n");
  
  h->priv.processing_cmd = CMD_SCAN;
  h->priv.processing_cmd_result = WIFI_TIMEOUT;
  h->callbacks.usart_send(send_buf, strlen(send_buf));
  
  wait_var_change(h, (char*) &h->priv.processing_cmd_result, (char*) &timeout, sizeof(h->priv.processing_cmd_result), false, 20000);
  
  if( h->priv.processing_cmd_result == WIFI_OK && ssid != 0)
  {
    desired_ap = (t_wifi_ap*) h->priv.process_buffer_out;
    
    if(strncmp(desired_ap->ssid, ssid, MAX_SSID_LEN) == 0 && desired_ap->active == true)
    {
      memcpy(r_ap, desired_ap, sizeof(t_wifi_ap));
    }
    else
    {
      ris = WIFI_SSID_NOT_FOUND;
      goto end;
    }
  }
  
  if(h->priv.processing_cmd_result == WIFI_ERROR && retry_count < 3)
  {
    h->callbacks.delay_ms(500);
    retry_count++;
    goto retry;
  }
  
  ris = h->priv.processing_cmd_result;
  
end:
  h->priv.processing_cmd = CMD_IDLE;
  
  return ris;
}

e_wifi_ris spwf01sa_factory_reset(t_wifi_spwf01sa_handle* h)
{
  e_wifi_ris ris;
  char send_buf[SEND_BUF_SIZE];
  const int timeout = WIFI_TIMEOUT;
  
  snprintf(send_buf, SEND_BUF_SIZE, "AT&F\r\n");
  h->priv.processing_cmd = CMD_GENERIC;
  h->priv.processing_cmd_result = WIFI_TIMEOUT;
  h->callbacks.usart_send(send_buf, strlen(send_buf));
    
  wait_var_change(h, (char*) &h->priv.processing_cmd_result, (char*) &timeout, sizeof(h->priv.processing_cmd_result), false, 1000);
  
  ris = h->priv.processing_cmd_result;
  
  h->priv.processing_cmd = CMD_IDLE;
  
  return ris;
}

e_wifi_ris spwf01sa_set_config(t_wifi_spwf01sa_handle* h, const char* config, const char* value)
{
  e_wifi_ris ris;
  char send_buf[SEND_BUF_SIZE];
  const int timeout = WIFI_TIMEOUT;
  
  snprintf(send_buf, SEND_BUF_SIZE, "AT+S.SCFG=%s,%s\r\n",config,value);
  h->priv.processing_cmd = CMD_GENERIC;
  h->priv.processing_cmd_result = WIFI_TIMEOUT;
  h->callbacks.usart_send(send_buf, strlen(send_buf));;
    
  wait_var_change(h, (char*) &h->priv.processing_cmd_result, (char*) &timeout, sizeof(h->priv.processing_cmd_result), false, 1000);
  
  ris = h->priv.processing_cmd_result;
  
  h->priv.processing_cmd = CMD_IDLE;
  
  return ris;
}

e_wifi_ris spwf01sa_get_config(t_wifi_spwf01sa_handle* h, const char* config, char* value, uint16_t maxlen)
{
  e_wifi_ris ris;
  char send_buf[SEND_BUF_SIZE];
  const int timeout = WIFI_TIMEOUT;
  
  memset(h->priv.process_buffer_out, 0, sizeof(h->priv.process_buffer_out));
  memset(h->priv.process_buffer_in, 0, sizeof(h->priv.process_buffer_in));
  
  snprintf(send_buf, SEND_BUF_SIZE, "AT+S.GCFG=%.40s\r\n",config);
  h->priv.processing_cmd = CMD_GET_CONFIG;
  h->priv.processing_cmd_result = WIFI_TIMEOUT;
  h->callbacks.usart_send(send_buf, strlen(send_buf));;
  
  snprintf(h->priv.process_buffer_in, sizeof(h->priv.process_buffer_out), "#  %.40s = ", config);
    
  wait_var_change(h, (char*) &h->priv.processing_cmd_result, (char*) &timeout, sizeof(h->priv.processing_cmd_result), false, 1000);
  
  if(h->priv.processing_cmd_result == WIFI_OK)
  {
    strncpy(value, h->priv.process_buffer_out, maxlen);
  }
  
  ris = h->priv.processing_cmd_result;
  
  h->priv.processing_cmd = CMD_IDLE;
  
  return ris;
}

/*
* Note: for some reason even if def is false, the module autoconnects at boot.
* Seems that data is saved during wifi reset
*/
e_wifi_ris spwf01sa_connect(t_wifi_spwf01sa_handle* h, t_wifi_ap* ap)
{
  e_wifi_ris ris;
  const e_wifi_ris timeout = WIFI_TIMEOUT;
  char secmode[10];
  bool wr;
  const bool c_true = true;
  char send_buf[SEND_BUF_SIZE];
  
  /* ap->sec to string */
  secmode[0] = ((ap->sec > 2) ? 2 : ap->sec) + '0';
  secmode[1] = 0;
  
  snprintf(send_buf, SEND_BUF_SIZE, "AT+S.SSIDTXT=%s\r\n",ap->ssid);
  h->priv.processing_cmd = CMD_GENERIC;
  h->priv.processing_cmd_result = WIFI_TIMEOUT;
  h->callbacks.usart_send(send_buf, strlen(send_buf));
  
  wait_var_change(h, (char*) &h->priv.processing_cmd_result, (char*) &timeout, sizeof(h->priv.processing_cmd_result), false, 200);
  
  if( h->priv.processing_cmd_result == WIFI_OK )
  {
    if(spwf01sa_set_config(h, "wifi_wpa_psk_text", ap->psw) == WIFI_OK)
    {
      if(spwf01sa_set_config(h, "wifi_priv_mode", secmode) == WIFI_OK)
      {
        if(spwf01sa_set_config(h, "wifi_mode", "1") == WIFI_OK)
        {
          snprintf(send_buf, SEND_BUF_SIZE, "AT&W\r\n");
          h->priv.processing_cmd = CMD_GENERIC;
          h->priv.processing_cmd_result = WIFI_TIMEOUT;
          
          h->callbacks.usart_send(send_buf, strlen(send_buf));
          
          wr = wait_var_change(h, (char*) &h->priv.processing_cmd_result, (char*) &timeout, sizeof(e_wifi_ris), false, 500);
          
          if( h->priv.processing_cmd_result == WIFI_OK )
          {
            /* Save ok */
            wifi_clear_flags(h);
            
            snprintf(send_buf, SEND_BUF_SIZE, "AT+CFUN=1\r\n");
            h->priv.processing_cmd = CMD_GENERIC;
            
            h->callbacks.usart_send(send_buf, strlen(send_buf));
            
            wr = wait_var_change(h, (char*) &h->priv.ready, (char*) &c_true, sizeof(bool), true, 15000);
            
            if( wr == true )
            {
              /* Resetted */
              wr = wait_var_change(h, (char*) &h->priv.sta_connected, (char*) &c_true, sizeof(bool), true, 20000);
              
              if(wr == false)
              {
                ris = WIFI_TIMEOUT;
                goto end;
              }
            }
            else
            {
              ris = WIFI_TIMEOUT;
              goto end;
            }
          }
          
        }
      }
    }
  }
  
  ris = h->priv.processing_cmd_result;
  
end:
  h->priv.processing_cmd = CMD_IDLE;
  
  return ris;
  
}

e_wifi_ris spwf01sa_save(t_wifi_spwf01sa_handle* h)
{
  e_wifi_ris ris;
  const e_wifi_ris timeout = WIFI_TIMEOUT;
  char send_buf[SEND_BUF_SIZE];
  
  snprintf(send_buf, SEND_BUF_SIZE, "AT&W\r\n");
  h->priv.processing_cmd = CMD_GENERIC;
  h->priv.processing_cmd_result = WIFI_TIMEOUT;
  
  h->callbacks.usart_send(send_buf, strlen(send_buf));
  
  wait_var_change(h, (char*) &h->priv.processing_cmd_result, (char*) &timeout, sizeof(h->priv.processing_cmd_result), false, 2000);
  
  ris = h->priv.processing_cmd_result;
  
  h->priv.processing_cmd = CMD_IDLE;
  
  return ris;
}

e_wifi_ris spwf01sa_disconnect(t_wifi_spwf01sa_handle* h)
{
  e_wifi_ris ris;
  bool wr;
  const e_wifi_ris timeout = WIFI_TIMEOUT;
  const bool c_false = false;
  char send_buf[SEND_BUF_SIZE];
  
  snprintf(send_buf, SEND_BUF_SIZE, "AT+S.SSIDTXT=\r\n");
  h->priv.processing_cmd = CMD_GENERIC;
  h->priv.processing_cmd_result = WIFI_TIMEOUT;
  h->callbacks.usart_send(send_buf, strlen(send_buf));
  
  wait_var_change(h, (char*) &h->priv.processing_cmd_result, (char*) &timeout, sizeof(h->priv.processing_cmd_result), false, 2000);
  
  if( h->priv.processing_cmd_result == WIFI_OK )
  {
    if(spwf01sa_set_config(h, "wifi_wpa_psk_text", "") == WIFI_OK)
    {
            
      snprintf(send_buf, SEND_BUF_SIZE, "AT+S.ROAM\r\n");
      h->priv.processing_cmd = CMD_GENERIC;
      h->priv.processing_cmd_result = WIFI_TIMEOUT;
      
      h->callbacks.usart_send(send_buf, strlen(send_buf));
      
      wait_var_change(h, (char*) &h->priv.processing_cmd_result, (char*) &timeout, sizeof(h->priv.processing_cmd_result), false, 2000);
      
      if(h->priv.processing_cmd_result == WIFI_OK)
      {
        
        wr = wait_var_change(h, (char*) &h->priv.sta_connected, (char*) &c_false, sizeof(bool), true, 2000);
        
        if(wr == true)
        {
          /* Disconnect ok */
          
          spwf01sa_save(h);
          
        }
        else
        {
          ris = WIFI_TIMEOUT;
          goto end;
        }
        
      }
    }
  }
  
  ris = h->priv.processing_cmd_result;
  
end:
  h->priv.processing_cmd = CMD_IDLE;
  
  return ris;
}

e_wifi_ris spwf01sa_open(t_wifi_spwf01sa_handle* h, char* remote, uint16_t port, char protocol, uint16_t* connid)
{
  e_wifi_ris ris;
  const uint16_t invalid_sock = 0xFFFF;
  const e_wifi_ris timeout = WIFI_TIMEOUT;
  char send_buf[SEND_BUF_SIZE];
  
  snprintf(send_buf, SEND_BUF_SIZE, "AT+S.SOCKON=%s,%d,%c\r\n", remote, port, protocol);

  memcpy(h->priv.process_buffer_out, &invalid_sock, sizeof(uint16_t));
  h->priv.processing_cmd = CMD_SOCKET_OPEN;
  h->priv.processing_cmd_result = WIFI_TIMEOUT;
  h->callbacks.usart_send(send_buf, strlen(send_buf));
  
  wait_var_change(h, (char*) &h->priv.processing_cmd_result, (char*) &timeout, sizeof(h->priv.processing_cmd_result), false, 5000);
  
  if(h->priv.processing_cmd_result == WIFI_OK)
  {
    memcpy(&connid, h->priv.process_buffer_out, sizeof(uint16_t));

    if(*connid == 0xFFFF)
    {
      ris = WIFI_TIMEOUT;
      goto end;
    }
  }
    
  
  ris = h->priv.processing_cmd_result;
  
end:
  h->priv.processing_cmd = CMD_IDLE;
  
  return ris;
  
}

e_wifi_ris spwf01sa_close(t_wifi_spwf01sa_handle* h, uint16_t connid)
{
  e_wifi_ris ris;
  uint16_t recv_len = 0;
  const e_wifi_ris timeout = WIFI_TIMEOUT;
  char send_buf[SEND_BUF_SIZE];
  
  do
  {
    recv_len = 0;
    ris = spwf01sa_recv(h, connid, 0, 100, &recv_len);
  }while(recv_len != 0);
  
  
  snprintf(send_buf, SEND_BUF_SIZE, "AT+S.SOCKC=%hd\r\n", connid);
  h->priv.processing_cmd = CMD_GENERIC;
  h->priv.processing_cmd_result = WIFI_TIMEOUT;
  h->callbacks.usart_send(send_buf, strlen(send_buf));
  
  wait_var_change(h, (char*) &h->priv.processing_cmd_result, (char*) &timeout, sizeof(h->priv.processing_cmd_result), false, 5000);
  
  ris = h->priv.processing_cmd_result;
  
  h->priv.processing_cmd = CMD_IDLE;
  
  return ris;
}

e_wifi_ris spwf01sa_send(t_wifi_spwf01sa_handle* h, uint16_t connid, const char* data, uint16_t len)
{
  e_wifi_ris ris;
  const e_wifi_ris timeout = WIFI_TIMEOUT;
  char send_buf[SEND_BUF_SIZE];
  
  snprintf(send_buf, SEND_BUF_SIZE, "AT+S.SOCKW=%hd,%hd\r", connid, len);
  h->priv.processing_cmd = CMD_GENERIC;
  h->priv.processing_cmd_result = WIFI_TIMEOUT;
  /* Send cmd */
  h->callbacks.usart_send(send_buf, strlen(send_buf));
  /* Send data */
  h->callbacks.usart_send(data, len);
  
  /* Wait OK */
  wait_var_change(h, (char*) &h->priv.processing_cmd_result, (char*) &timeout, sizeof(h->priv.processing_cmd_result), false, 5000);
  
  if(h->priv.processing_cmd_result == WIFI_TIMEOUT)
  {
    /* If some char are lost during uart transfer, module remains stuck. We send some byte to try a recovery */
    h->callbacks.usart_send("FLUSH", 5);
    wait_var_change(h, (char*) &h->priv.processing_cmd_result, (char*) &timeout, sizeof(h->priv.processing_cmd_result), false, 500);
    
    if(h->priv.processing_cmd_result == WIFI_OK)
    {
      ris = WIFI_ERROR;
      goto end;
    }
    
  }
  ris = h->priv.processing_cmd_result;
  
end:
  h->priv.processing_cmd = CMD_IDLE;
  
  return ris;
}

e_wifi_ris spwf01sa_recv(t_wifi_spwf01sa_handle* h, uint16_t connid, char* data, uint16_t len, uint16_t* recv_len)
{
  e_wifi_ris ris;
  uint16_t tmplen;
  const e_wifi_ris timeout = WIFI_TIMEOUT;
  char send_buf[SEND_BUF_SIZE];
  
  *recv_len = 0;
  memset(h->priv.process_buffer_out, 0, sizeof(h->priv.process_buffer_out));
  memset(h->priv.process_buffer_in, 0, sizeof(h->priv.process_buffer_in));
  h->priv.process_buffer_out_index = 0;

  snprintf(send_buf, SEND_BUF_SIZE, "AT+S.SOCKQ=%hd\r", connid);
  h->priv.processing_cmd = CMD_QUERY_SOCKET_DATALEN;
  h->priv.processing_cmd_result = WIFI_TIMEOUT;

  h->callbacks.usart_send(send_buf, strlen(send_buf));
  
  wait_var_change(h, (char*) &h->priv.processing_cmd_result, (char*) &timeout, sizeof(h->priv.processing_cmd_result), false, 500);
  
  if(h->priv.processing_cmd_result == WIFI_OK)
  {
    memcpy(&tmplen, h->priv.process_buffer_out, sizeof(uint16_t));

    if(tmplen > 0 && tmplen <= len )
    {
      
      h->priv.process_buffer_out_recv_len = tmplen;

      snprintf(send_buf, SEND_BUF_SIZE, "AT+S.SOCKR=%hd,%hd\r", connid, tmplen);

      h->priv.processing_cmd = CMD_RECEIVE_DATA;
      h->priv.processing_cmd_result = WIFI_TIMEOUT;
      
      h->callbacks.usart_send(send_buf, strlen(send_buf));
      
      /* Wait OK */
      wait_var_change(h, (char*) &h->priv.processing_cmd_result, (char*) &timeout, sizeof(h->priv.processing_cmd_result), false, 5000);
      
      if(h->priv.processing_cmd_result == WIFI_OK)
      {
        if(data != 0)
          memcpy(data, h->priv.process_buffer_out, len);
        *recv_len = tmplen;
      }
    }
    else
    {
      *recv_len = 0;
      ris = WIFI_OK;
      goto end;
    }
  }
 

  ris = h->priv.processing_cmd_result;
  
end:
  h->priv.processing_cmd = CMD_IDLE;
  
  return ris;
}

e_wifi_ris spwf01sa_get_version(t_wifi_spwf01sa_handle* h, char* v, uint16_t max_size)
{
  e_wifi_ris ris;
  char send_buf[SEND_BUF_SIZE];
  const int timeout = WIFI_TIMEOUT;

  memset(h->priv.process_buffer_out, 0, sizeof(h->priv.process_buffer_out));
  memset(h->priv.process_buffer_in, 0, sizeof(h->priv.process_buffer_in));

  snprintf(send_buf, SEND_BUF_SIZE, "AT+S.STS=version\r\n");
  h->priv.processing_cmd = CMD_GET_STATUS;
  h->priv.processing_cmd_result = WIFI_TIMEOUT;
  h->callbacks.usart_send(send_buf, strlen(send_buf));;

  snprintf(h->priv.process_buffer_in, sizeof(h->priv.process_buffer_out), "#  version = ");

  wait_var_change(h, (char*) &h->priv.processing_cmd_result, (char*) &timeout, sizeof(h->priv.processing_cmd_result), false, 1000);

  if(h->priv.processing_cmd_result == WIFI_OK)
  {
    strncpy(v, h->priv.process_buffer_out, max_size);
  }

  ris = h->priv.processing_cmd_result;

  h->priv.processing_cmd = CMD_IDLE;

  return ris;
}

void spwf01sa_setup(t_wifi_spwf01sa_handle* handle)
{ 
    at_setup(&handle->priv.at_handle, "\r\n", handle->priv.at_cache, sizeof(handle->priv.at_cache));

    handle->usart_fetch_byte = usart_fetch_byte;

}
