#ifndef PLATFORM_WIFI_H
#define PLATFORM_WIFI_H

#include <stdint.h>
#include <atbox/net/wifi/spwf01sa/spwf01sa.h>

extern t_wifi_spwf01sa_handle wifi_handle;

uint16_t new_wifi_data_cb(char* data, uint16_t len);

extern t_wifi_spwf01sa_handle wifi_handle;
#endif