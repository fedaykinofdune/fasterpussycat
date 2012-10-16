#include "lua_http_client.h"

static int process_response_ref=0;

void register_http_client(){
  /* register target */

  static const struct luaL_reg http_client_m [] = {
    {"__index", l_arb_getprop},
    {"__newindex", l_arb_setprop},
    {"new_request", l_new_http_request},
    {NULL, NULL}
  };

  static const struct luaL_reg http_request_m [] = {
    {"__index", l_arb_getprop},
    {"__newindex", l_arb_setprop},
    {"get_host", l_request_get_host},
    {"get_path", l_request_get_path},
    {"get_path", l_request_get_path_only},
    {NULL, NULL}
  };


  static const struct luaL_reg http_response_m [] = {
    {"__index", l_arb_getprop},
    {"__newindex", l_arb_setprop},
    {"get_body", l_response_get_body},
    {"get_code", l_response_get_code},
    {NULL, NULL}
  };


  register_faster_funcs("http_client",http_client_m);
  register_faster_funcs("http_request",http_request_m);
  register_faster_funcs("http_response",http_request_m);
}

static unsigned char process_lua_request(struct http_request *req, struct http_response *res){
  if(!process_response_ref){
   lua_getfield(L,LUA_REGISTRYINDEX, "faster.http_client");
   lua_getfield(L,LUA_REGISTRYINDEX, "process_response_callback");
   process_response_ref=luaL_ref(L, LUA_REGISTRYINDEX);
   lua_remove(L,-1);
  }
  lua_rawgeti(L, LUA_REGISTRYINDEX, process_response_ref);
  lua_rawgeti(L, LUA_REGISTRYINDEX, req->lua_ref);
  lua_rawgeti(L, LUA_REGISTRYINDEX, res->lua_ref);
  luaL_unref (L, LUA_REGISTRYINDEX, req->lua_ref);
  luaL_unref (L, LUA_REGISTRYINDEX, res->lua_ref);
  int rc=lua_pcall(L,2,0,0);
  if(rc){
    const char *s=lua_tostring(L,-1);
    printf("ERROR: %s\n", s);
  }
  return 1;
}

int l_new_http_request(lua_State *L){
  const char *url=luaL_checkstring(L,1);
  char *method=NULL;
  if(!lua_isnil(L,2)) {
    if(!lua_istable(L,2)) luaL_error(L,"Options expected");
    
    lua_getfield(L,2,"method");
    if(!lua_isnil(L,-1)) {
      if(!lua_isstring(L,-1)) luaL_error(L, "method must be string");
      method=ck_strdup(lua_tostring(L,-1));
    }
    lua_pop(L,1);
  }

  struct http_request *req=lua_newuserdata(L, sizeof(struct http_request));
  req->callback=process_lua_request;
  parse_url(url,req, 0);
  if(!method) method=ck_strdup("GET");
  req->method=method;
  setup_obj_env();
  get_faster_value(L, "http_request");
  lua_setmetatable(L, -2);
  return 1;
}

int l_request_get_host(lua_State *L){
  struct http_request *req=check_http_request(L, 1);

  lua_pushstring(L,req->host);
  return 1;
}



int l_request_get_path_only(lua_State *L){
  struct http_request *req=check_http_request(L, 1);
  lua_pushstring(L,path_only(req));
  return 1;
}



int l_request_get_path(lua_State *L){
  struct http_request *req=check_http_request(L, 1);
  lua_pushstring(L,serialize_path(req,0,0));
  return 1;
}


int l_response_get_body(lua_State *L){
  struct http_response *res=check_http_response(L, 1);
  lua_pushlstring(L,res->payload,res->pay_len);
  return 1;
}



int l_response_get_code(lua_State *L){
  struct http_response *res=check_http_response(L, 1);
  lua_pushinteger(L,res->code);
  return 1;
}


struct http_response *raw_new_http_response(lua_State *L){
  struct http_response *res=lua_newuserdata(L, sizeof(struct http_response));
  setup_obj_env();
  get_faster_value(L, "http_response");
  lua_setmetatable(L, -2);
  return res;
}



struct http_request *check_http_request(lua_State *L, int pos){
   void *ud = luaL_checkudata(L, pos, "faster.http_request");
   luaL_argcheck(L, ud != NULL, pos, "`faster.http_request' expected");
   return ((struct http_request *) ud);
}



struct http_response *check_http_response(lua_State *L, int pos){
   void *ud = luaL_checkudata(L, pos, "faster.http_response");
   luaL_argcheck(L, ud != NULL, pos, "`faster.http_response' expected");
   return ((struct http_response *) ud);
}


int l_async_request(lua_State *L){
  struct http_request *req=check_http_request(L,1);
  lua_pushvalue(L,1);
  req->lua_ref=luaL_ref (L, LUA_REGISTRYINDEX);
  async_request(req);
  lua_pushboolean(L,1);
  return 1;
}

