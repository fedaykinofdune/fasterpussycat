#ifndef FASTERPUSSYCAT_LUA_HTTP_CLIENT_H
#define FASTERPUSSYCAT_LUA_HTTP_CLIENT_H


#include "lua.h"
#include "http_client.h"


/* lua_http_client.c */
void register_http_client(void);
int l_new_http_request(lua_State *L);
int l_request_get_host(lua_State *L);
int l_request_get_path_only(lua_State *L);
int l_request_get_path(lua_State *L);
int l_response_get_body(lua_State *L);
int l_response_get_code(lua_State *L);
struct http_response *raw_new_http_response(lua_State *L);
struct http_request *check_http_request(lua_State *L, int pos);
struct http_response *check_http_response(lua_State *L, int pos);
int l_async_request(lua_State *L);



#endif
