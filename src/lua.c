#include "lua.h"
#include "match_rule.h"

lua_State *L;

void register_faster_funcs(char *name, const luaL_reg *funcs){
  char *newtable=malloc(strlen(name)+8);
  strcat(newtable,"faster.");
  strcat(newtable, name);
  lua_getglobal(L,"faster");
  lua_pushstring(L, name);
  luaL_newmetatable(L,newtable);
  free(newtable);
  lua_pushstring(L, "__index");
  lua_pushvalue(L, -2);  /* pushes the metatable */
  lua_settable(L, -3);  /* metatable.__index = metatable */
  luaL_openlib(L, NULL, funcs, 0);
  set_faster_value(L,name);
}


void setup_obj_env(){
  lua_newtable(L);
  lua_setfenv(L,-2);
}

void setup_lua(){
  L = lua_open();   /* opens Lua */
  luaopen_base(L);             /* opens the basic library */
  luaopen_table(L);            /* opens the table library */
  luaopen_io(L);               /* opens the I/O library */
  luaopen_string(L);           /* opens the string lib. */
  luaopen_math(L);             /* opens the math lib. */

  lua_newtable(L);
  lua_setglobal(L,"faster");
  register_matching();
  register_http_client();
}


void add_target(char *target){
  int rc;
  get_faster_value(L, "add_target");
  lua_pushstring(L, target);
  rc=lua_pcall(L,1,0,0);
  if(rc) printf("error: %s\n", lua_tostring(L, -1));
}

void get_faster_value(lua_State *L, char *n){
  lua_getglobal(L, "faster");
  lua_getfield(L,-1,n);
  lua_remove(L,-2);
}



void set_faster_value(lua_State *L, char *n){
  lua_getglobal(L, "faster");
  lua_pushvalue(L,-2);
  lua_setfield(L,-2, n);
  lua_pop(L,1);
  lua_pop(L,1);
}

unsigned char lua_callback_evaluate(struct http_request *req, struct http_response *rep, void *data){
  int rc;
  struct lua_callback *callback= (struct lua_callback*) data;
  lua_rawgeti(L, LUA_REGISTRYINDEX, callback->callback_ref);
  lua_rawgeti(L, LUA_REGISTRYINDEX, req->lua_ref);
  lua_rawgeti(L, LUA_REGISTRYINDEX, rep->lua_ref);
  lua_rawgeti(L, LUA_REGISTRYINDEX, callback->data_ref);
  lua_call(L, 3, 1);
  if(lua_isnil(L,-1)){
    lua_pop(L,1);
    return DETECT_NEXT_RULE;
  }
  rc=lua_toboolean(L,-1);
  lua_pop(L,1);
  return (rc ? DETECT_SUCCESS : DETECT_FAIL);
  
}


int l_arb_setprop(lua_State *L){
  lua_getfenv(L, 1);
  lua_pushvalue(L,2);
  lua_pushvalue(L,3);
  lua_settable(L,-3);
  return 0;
}



int l_arb_getprop(lua_State *L){
  lua_getfenv(L, 1);
  lua_pushvalue(L,2);
  lua_gettable(L,-2);
  if(lua_isnil(L,-1)){
    lua_pop(L,1);
    lua_getmetatable(L,1);
    lua_pushvalue(L,2);
    lua_gettable(L,-2);
  }
  return 1;
}

static struct target *check_target(lua_State *L){
   void *ud = luaL_checkudata(L, 1, "faster.target");
   luaL_argcheck(L, ud != NULL, 1, "`faster.target' expected");
   return *((struct target **) ud);
}



static struct match_ruleset *check_ruleset(lua_State *L){
   void *ud = luaL_checkudata(L, 1, "faster.ruleset");
   luaL_argcheck(L, ud != NULL, 1, "`faster.ruleset' expected");
   return (struct match_ruleset *) ud;
}


void do_lua_callback(struct lua_callback *callback, struct http_request *req, struct http_response *rep){
  lua_rawgeti(L, LUA_REGISTRYINDEX, callback->callback_ref);
  lua_rawgeti(L, LUA_REGISTRYINDEX, req->lua_ref);
  lua_rawgeti(L, LUA_REGISTRYINDEX, rep->lua_ref);
  lua_rawgeti(L, LUA_REGISTRYINDEX, callback->data_ref);
  lua_call(L, 3, 2);
}



void free_lua_callback(struct lua_callback *callback){
  if(!callback) return;
  luaL_unref(L, LUA_REGISTRYINDEX, callback->callback_ref);
  luaL_unref(L, LUA_REGISTRYINDEX, callback->data_ref);
  ck_free(callback);
}
