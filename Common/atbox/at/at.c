#include <atbox/at/at.h>
#include <string.h>


#define CLEAR_CACHE  memset(handle->priv.cache_cmd, 0, handle->priv.cache_size); \
                    handle->priv.cache_index = 0;

/*
* Function:  at_process_match 
* -----------------------------
* Processing incoming data and cache every byte into the at handle.
* Will notify with return code when the terminator string is matched
* When that happends the cache MUST be cleared with at_clear_cache
*
*  byte: new byte reiceved
*  m_list: list of command to match
*  len: size of list
*
*  returns: the code parameter of m_list 
*           AT_DUMMY if dummy match or no match yet
*           AT_UNKNOWN if terminator matched but the string is not in m_list
*/
uint16_t at_process_match(t_at_handle* handle, char byte, const t_at_match* m_list, uint16_t len)
{
  uint8_t i;
  
  /* Cache as mush as possibile until at terminator received.
  * Leave 1 additional byte to be 100% always 0 to prevent memory overflow of std functions*/
  if(handle->priv.cache_index < (handle->priv.cache_size - 2))
    handle->priv.cache_cmd[handle->priv.cache_index++] = byte;
  else
  {
	  CLEAR_CACHE
  }

  if(handle->priv.m_index <handle->priv.endline_len && (byte == handle->priv.endline[handle->priv.m_index]))
  {
    handle->priv.m_index++;
    
    if(handle->priv.m_index == handle->priv.endline_len)
    {
      handle->priv.m_index = 0;
      
      /* Now find matched string */
      for(i = 0; i < len; i++)
      {
        if(m_list[i].str != 0 && strstr(handle->priv.cache_cmd,m_list[i].str) == handle->priv.cache_cmd)
        {
          return m_list[i].code;
        }
      }

      if(strlen(handle->priv.cache_cmd) <= handle->priv.endline_len)
      {
    	  CLEAR_CACHE
    	  return AT_DUMMY;
      }
      
      return AT_UNKNOWN;
    }
  }
  else
  {
    handle->priv.m_index = 0;
  }
  
  return AT_DUMMY;
}

char* at_get_cmd_cache(t_at_handle* handle)
{
  return handle->priv.cache_cmd;
}

void at_clear_cache(t_at_handle* handle)
{
  memset(handle->priv.cache_cmd, 0, handle->priv.cache_size);
  handle->priv.cache_index = 0;
}

/*
* Function:  at_setup
* -----------------------------
* Initialize a new at handle (must be allocated or declared before).
* Must be the first operation before using the handle
*
*  match_terminator: at endline to match, usually "\r\n", but also ":" may be useful
*  cache_cmd: an allocated/declared buffer to use for caching command
*  cache_size: size of the cache
*
*/
void at_setup(t_at_handle* handle, const char* match_terminator, char* cache_cmd, uint32_t cache_size)
{
  handle->priv.m_index = 0;
  handle->priv.endline = match_terminator;
  handle->priv.endline_len = strlen(match_terminator);
  handle->priv.cache_cmd = cache_cmd;
  handle->priv.cache_size = cache_size;

}
