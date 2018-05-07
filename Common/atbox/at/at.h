#ifndef ATBOX_PARSER_AT_AT_H
#define ATBOX_PARSER_AT_AT_H

#include <stdint.h>
#include <stdbool.h>

#define AT_DUMMY 0
#define AT_OK 1
#define AT_ERROR 2
#define AT_FAIL 3
#define AT_UNKNOWN 512

#define AT_ADD_MATCH(s,c) {(s),(sizeof(s)), c},

typedef struct{
  char* str;
  uint8_t len;
  uint16_t code;
}t_at_match;

typedef struct{
  char* cache_cmd;
  uint32_t cache_size;
  uint32_t cache_index;
  uint16_t m_index;
  const char* endline;
  uint8_t endline_len;
}t_at_private;


typedef struct{
  t_at_private priv;
}t_at_handle;


uint16_t at_process_match(t_at_handle* handle, char byte, const t_at_match* m_list, uint16_t len);
char* at_get_cmd_cache(t_at_handle* handle);
void at_clear_cache(t_at_handle* handle);
void at_setup(t_at_handle* handle, const char* match_terminator, char* cache_cmd, uint32_t max_cache_size);

#endif