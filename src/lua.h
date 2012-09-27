
#ifndef FASTERPUSSYCAT_LUA_H
#define FASTERPUSSYCAT_LUA_H
#include "db.h"
#include "engine.h"
#include "lua51/lua.h"
#include "lua51/lauxlib.h" 
extern lua_State *L;

/* lua.c */
void setup_lua(void);

struct lua_callback {
  int callback_ref;
  int data_ref;
};
#endif
