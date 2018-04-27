#ifndef ATBOX_WIFI_INTERFACE_h
#define ATBOX_WIFI_INTERFACE_h

#include <stdint.h>
#include <stdbool.h>

#define MAX_SSID_LEN 32
#define MAX_KEY_SIZE 64
#define MAX_MAC_SIZE 6

typedef enum{
  WIFI_OK = 0,
  WIFI_TIMEOUT = -1,
  WIFI_BUSY = -2,
  WIFI_ERROR = -4,
  WIFI_ALREADYCONNECTED = -5,
  WIFI_SSID_NOT_FOUND = -6,
  ESP_UNSUPPORTED = -126,
  ESP_UNKNOWN = -127
}e_wifi_ris;

typedef enum{
  WIFI_SEC_OPEN = 0,
  WIFI_SEC_WEP = 1,
  WIFI_SEC_WPA = 2,
  WIFI_SEC_WPA2 = 3,
}e_wifi_sec;

typedef enum{
  WIFI_STA = 1,
  WIFI_AP = 2,
  WIFI_STA_AP = 3,
}e_wifi_mode;

typedef enum{
  WIFI_TCP = 1,
  WIFI_UDP = 2,
  WIFI_SSL = 3,
}e_wifi_sock;

typedef enum{
  WIFI_READY,
  WIFI_CONNECTED,
  WIFI_GOT_IP,
  WIFI_DISCONNECTED,
  WIFI_CONNECTION_OPENED,
  WIFI_CONNECTION_CLOSED,
  WIFI_NEW_PACKET,
  WIFI_FOUND_AP,
  WIFI_UNSUPPORTED,
}e_wifi_cb;

typedef struct{
  int16_t rssi;
  char ssid[MAX_SSID_LEN];
  char psw[MAX_KEY_SIZE];
  uint8_t mac[MAX_MAC_SIZE];
  uint8_t channel;
  e_wifi_sec sec;
  bool active;
}t_wifi_ap;

typedef struct {
  uint8_t conn_id;
  uint16_t len;
  char* data;
}t_wifi_packet;

#endif