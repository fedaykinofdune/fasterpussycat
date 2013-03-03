#ifndef FASTERPUSSYCAT_SHOGGOTH_API_H
#define FASTERPUSSYCAT_SHOGGOTH_API_H
#include <stdint.h>
#include "../common/simple_buffer.h"

typedef struct {
  simple_buffer *headers;
  simple_buffer *path;
  simple_buffer *method;
  simple_buffer *body;
  simple_buffer *host;
  uint16_t port;
  uint32_t options;
  uint32_t handle;
} http_request;

/* shoggoth_api.c */
void register_shoggoth_api(lua_State *L);
int l_connect_endpoint(lua_State *L);
int l_poll(lua_State *L);
void l_raw_poll(lua_State *L, void *sock);
int l_enqueue_http_request(lua_State *L);
#endif

