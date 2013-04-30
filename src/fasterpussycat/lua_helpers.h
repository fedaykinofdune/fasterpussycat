#ifndef FASTERPUSSYCAT_LUA_HELPERS_H
#define FASTERPUSSYCAT_LUA_HELPERS_H
#include <lua5.1/lua.h>
#include <lua5.1/lauxlib.h>
#include <lua5.1/lualib.h>
extern lua_State *l_state;
void lua_initialize();


#define FP_LUA_PATH "/home/caleb/src/fasterpussycat/src/fasterpussycat/lua/"

#endif
