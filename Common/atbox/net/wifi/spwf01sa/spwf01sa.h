#ifndef ATBOX_SPWF01SA_H
#define ATBOX_SPWF01SA_H

#include <atbox/net/wifi/wifi_base.h>
#include <atbox/at/at.h>

#define AT_CACHE_SIZE 512
#define CB_BUFFER_SIZE 512
#define PROCESS_BUFFER_OUT_SIZE 4096
#define PROCESS_BUFFER_IN_SIZE 512

typedef struct {
  void (*delay_ms)(uint32_t);
  uint32_t (*get_tick_ms)();
  uint8_t (*gpio_write_reset)(bool);
  bool (*usart_send)(const char*, uint16_t);
  void (*event_callback)(e_wifi_cb, char*, uint16_t);

  /* RTOS */
  /* This callback is called when a new event is detected,
     if the delay functions is sleeping to allow other thread to run
     this function allows to stop sleeping. ex. sleeping with WaitForEvent(timeout) */
  void (*os_quit_sleep)();
}t_wifi_spwf01sa_cb;

typedef struct {
  t_at_handle at_handle;
  char at_cache[AT_CACHE_SIZE];
  uint8_t processing_cmd;
  e_wifi_ris processing_cmd_result;

  char process_buffer_in[PROCESS_BUFFER_IN_SIZE];
  char process_buffer_out[PROCESS_BUFFER_OUT_SIZE];
  uint16_t process_buffer_out_index;
  uint16_t process_buffer_out_recv_len;
  char cb_buffer[CB_BUFFER_SIZE];
  bool poweron;
  bool ready;
  bool sta_connected;
  uint8_t ip[4];
}t_wifi_spwf01sa_private;

typedef struct {

  t_wifi_spwf01sa_cb callbacks;
  t_wifi_spwf01sa_private priv;

  void (*usart_fetch_byte)(void*, char);

}t_wifi_spwf01sa_handle;


e_wifi_ris spwf01sa_close(t_wifi_spwf01sa_handle* h, uint16_t connid);
e_wifi_ris spwf01sa_connect(t_wifi_spwf01sa_handle* h, t_wifi_ap* ap);
e_wifi_ris spwf01sa_disconnect(t_wifi_spwf01sa_handle* h);
e_wifi_ris spwf01sa_get_version(t_wifi_spwf01sa_handle* h, char* v, uint16_t max_size);
e_wifi_ris spwf01sa_get_config(t_wifi_spwf01sa_handle* h, const char* config, char* value, uint16_t maxlen);
e_wifi_ris spwf01sa_set_config(t_wifi_spwf01sa_handle* h, const char* config, const char* value);
e_wifi_ris spwf01sa_init(t_wifi_spwf01sa_handle* h, bool factory_reset);
e_wifi_ris spwf01sa_open(t_wifi_spwf01sa_handle* h, char* remote, uint16_t port, char protocol, uint16_t* connid);
e_wifi_ris spwf01sa_send(t_wifi_spwf01sa_handle* h, uint16_t conn_index, const char* data, uint16_t len);
e_wifi_ris spwf01sa_recv(t_wifi_spwf01sa_handle* h, uint16_t connid, char* data, uint16_t len, uint16_t* recv_len);
e_wifi_ris spwf01sa_scan(t_wifi_spwf01sa_handle* h, const char* ssid, t_wifi_ap* r_ap);
e_wifi_ris spwf01sa_factory_reset(t_wifi_spwf01sa_handle* h);
e_wifi_ris spwf01sa_wait_ready(t_wifi_spwf01sa_handle* h);
void spwf01sa_setup(t_wifi_spwf01sa_handle* handle);

/* Keep private */
#undef AT_CACHE_SIZE
#undef CB_BUFFER_SIZE
#undef PROCESS_BUFFER_OUT_SIZE
#undef PROCESS_BUFFER_IN_SIZE

#endif
