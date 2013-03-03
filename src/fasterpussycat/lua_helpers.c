#include <lua5.1/lua.h>
#include <lua5.1/lauxlib.h>
#include <lua5.1/lualib.h>
#include "lua_helpers.h"
#include "shoggoth_api.h"

lua_State *l_state;

void lua_initialize(){
  l_state=luaL_newstate();
  luaL_openlibs(l_state);
  register_shoggoth_api(l_state);
}

