#ifndef M1_MOUSERKIT_H
#define M1_MOUSERKIT_H

#include <stdbool.h>
#include <stdint.h>
#include "MQTTClient.h"

typedef struct{
	uint32_t rate_s;
	uint32_t aggr_rate_s;
	bool heartbeat_active;
	float min_temp;
	float max_temp;
	float min_hum;
	float max_hum;
	float min_press;
	float max_press;
	int32_t max_accx;
	int32_t min_accx;
	int32_t max_accy;
	int32_t min_accy;
	int32_t max_accz;
	int32_t min_accz;
	int32_t max_gyrox;
	int32_t min_gyrox;
	int32_t min_gyroy;
	int32_t max_gyroy;
	int32_t min_gyroz;
	int32_t max_gyroz;
	int32_t max_magx;
	int32_t min_magx;
	int32_t max_magy;
	int32_t min_magy;
	int32_t max_magz;
	int32_t min_magz;
}t_m1_config;

typedef struct{
    char ssid[32];	/**< SSID of the WIFI network that your using for internet connectivity.  */
    char auth[8];	/**< Authentication type for Wifi (e.g. WEP/WPA2 etc.) */
    char password[32];	/**< Wifi security key corresponding to your Wifi SSID */
    char m1_project_mqtt_id[128];
    char m1_user_id[128];
    char m1_user_password[128];
    char m1_api_key[128];
    char m1_device[128];
    uint32_t magic;
    uint8_t pad[4];
}t_m1_net_config;

typedef struct{
  uint8_t pub_topic[128];  /**< topic to publish */
  uint8_t sub_topic[128];  /**< topic to subscribe */
  uint8_t clientid[64];    /**< topic to publish */
  enum QoS qos;            /**< Quality of service parameter for MQTT connection to IBM Watson IOT platform service */
  uint8_t username[64];    /**< "use-token-auth"*/
	uint8_t password[64];    /**< secret token generated (or the one you passed) while registering your device wih â€œIBM Watson IOT Platformâ€� service  */
	uint8_t hostname[64];    /**< your_ibm_org_id."messaging.internetofthings.ibmcloud.com"  */
  uint8_t device_type[64]; /**< device type in IBM Watson IOT platform service */
  uint8_t org_id[64];      /**< org id in IBM Watson */
  uint32_t port;           /**< TCP port */
  uint8_t protocol;        /**<t -> tcp , s-> secure tcp, c-> secure tcp + certificates */
}t_m1_mqtt;

#define M1_NET_CONFIG_SIZE (128*5 + 32 + 8 + 32 + 4 + 4)



extern t_m1_config m1_config;
extern t_m1_net_config m1_net_config;
extern t_m1_mqtt m1_mqtt;


void m1_generate_json(char* json, int size);
void m1_process_messages(char* json);
void m1_generate_json_aggr(char* json, int size);
void m1_inc_btn_pres();
void m1_poll_sensors();
bool m1_has_threshold();

void m1_init();
void m1_dump_parameter();
void m1_ask_net_parameters();
void m1_config_mqtt( uint8_t *macadd );

extern void *ACCELERO_handle;
extern void *GYRO_handle;
extern void *MAGNETO_handle;
extern void *HUMIDITY_handle;
extern void *TEMPERATURE_handle;
extern void *PRESSURE_handle;

#endif
