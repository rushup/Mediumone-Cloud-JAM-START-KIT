#ifndef PLATFORM_MQTT_H
#define PLATFORM_MQTT_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/**
  * @brief  timer structure
  */	
typedef struct timer {
	unsigned long systick_period;
	unsigned long end_time;
}Timer;

/**
  * @brief network structure, contain socket id, and pointer to MQTT read,write and disconnect functions 
  */	
typedef struct network
{
	int my_socket;
	int (*mqttread) (struct network*, unsigned char*, int, int);
	int (*mqttwrite) (struct network*, unsigned char*, int, int);
        void (*disconnect) (struct network*);
}Network;

void SysTickIntHandlerMQTT(void);
char expired(Timer*);
void countdown_ms(Timer*, unsigned int);
void countdown(Timer*, unsigned int);
int left_ms(Timer*);
void InitTimer(Timer*);

void NewNetwork(Network*);   
int mqtt_open_socket(Network* net, char* hostname, uint32_t port_number, char protocol);
int mqtt_socket_write (Network*, unsigned char*, int, int);
int mqtt_socket_read (Network*, unsigned char*, int, int);
void mqtt_ocket_close (Network* n);


#endif