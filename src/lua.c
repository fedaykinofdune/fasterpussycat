#include "lua.h"
#include "engine.h"
lua_State *L;

void setup_lua(){
  L = lua_open();   /* opens Lua */
  luaopen_base(L);             /* opens the basic library */
  luaopen_table(L);            /* opens the table library */
  luaopen_io(L);               /* opens the I/O library */
  luaopen_string(L);           /* opens the string lib. */
  luaopen_math(L);             /* opens the math lib. */
  static const struct luaL_reg faster [] = {
    {"targets", l_targets},
    {NULL, NULL}  /* sentinel */
  };


  /* register target */


  static const struct luaL_reg target_m [] = {
    {"host", l_target_host},
    {"port", l_target_port},
    {"path", l_target_path},
    {"ssl", l_target_ssl},
    {"queue", l_target_request},
    {NULL, NULL}
  };

  luaL_newmetatable(L, "faster.target");
  lua_pushstring(L, "__index");
  lua_pushvalue(L, -2);  /* pushes the metatable */
  lua_settable(L, -3);  /* metatable.__index = metatable */
  luaL_openlib(L, NULL, target_m, 0);
  luaL_openlib(L, NULL, target_f, 0);
}

static int l_targets (lua_State *L) {
  struct target *t;
  lua_newtable(L);
  i=1;
  for(t=get_targets();t;t=t->hh.next){
    target2lua(L,t);
    lua_pushinteger(L,i);
    lua_settable(L,-3);
    i++;
  }
  return 1;
}

static void target2lua(lua_State *L, struct target *t) {
  struct target **ptrptr=(struct target **) lua_newuserdata(L, sizeof(struct target *));
  *ptrptr=t;
  luaL_getmetatable(L, "faster.target");
  lua_setmetatable(L, -2);
}

static struct target *check_target(lua_State *L){
   void *ud = luaL_checkudata(L, 1, "faster.target");
   luaL_argcheck(L, ud != NULL, 1, "`faster.target' expected");
   return *((struct target **) ud);
}

static int l_target_host(lua_State *L){
  struct target *t=check_target(L);
  lua_pushstring(L, t->host);
  return 1;
}



static int l_target_port(lua_State *L){
  struct target *t=check_target(L);
  lua_pushinteger(L, t->prototype_request->port);
  return 1;
}


static int l_target_path(lua_State *L){
  struct target *t=check_target(L);
  lua_pushstring(L, serialize_path(t->prototype_request,0,0));
  return 1;
}

static int l_target_ssl(lua_State *L){
  struct target *t=check_target(L);
  lua_pushinteger(L, t->prototype_request->proto==2);
  return 1;
}


