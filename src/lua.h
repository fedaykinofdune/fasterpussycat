#ifndef FASTERPUSSYCAT_LUA_H
#define FASTERPUSSYCAT_LUA_H
#include "lua5.1/lua.h"
#include "lua5.1/lualib.h"
#include "lua5.1/lauxlib.h" 
#include "http_client.h"
extern lua_State *L;
struct http_request;
struct http_response;

struct lua_callback {
  int callback_ref;
  int data_ref;
};

/* lua.c */
void register_faster_funcs(char *name, const luaL_Reg *funcs);
void setup_obj_env(void);
void setup_lua(void);
void get_faster_value(lua_State *L, char *n);
void set_faster_value(lua_State *L, char *n);
unsigned char lua_callback_evaluate(struct http_request *req, struct http_response *rep, void *data);
int l_arb_setprop(lua_State *L);
int l_arb_getprop(lua_State *L);
void do_lua_callback(struct lua_callback *callback, struct http_request *req, struct http_response *rep);
void free_lua_callback(struct lua_callback *callback);
void add_target(char *target);
#endif
