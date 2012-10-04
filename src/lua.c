#include "lua.h"
#include "engine.h"
lua_State *L;

void register_faster_funcs(char *name, const luaL_reg[] funcs){
  char *newtable=malloc(strlen(name)+8);
  strcat(newtable,"faster.");
  strcat(newtable, name);
  lua_getglobal(L,"faster");
  lua_pushstring(name);
  luaL_newmetatable(L,newtable);
  free(newtable);
  lua_pushstring(L, "__index");
  lua_pushvalue(L, -2);  /* pushes the metatable */
  lua_settable(L, -3);  /* metatable.__index = metatable */
  luaL_openlib(L, NULL, funcs, 0);
  lua_settable(L,-3);
}


void setup_obj_env(){
  lua_newtable(L);
  lua_fsetenv(L,-2);
}

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



  static const struct luaL_reg ruleset_m [] = {
    {"new_rule", l_new_rule},
    {NULL, NULL}
  };
  

  lua_newtable(L);
  lua_setglobal(L,"faster");

  register_faster_funcs("ruleset", ruleset_m);

  

  default_ruleset=lua_newuserdata(L, sizeof(struct match_ruleset));
  luaL_getmetatable(L, "faster.ruleset");
  lua_setmetatable(L, -2);
  lua_setglobal (L, "faster.default_rules");
 
  lua_pushinteger (L, MATCH_200);
  lua_setglobal (L, "faster.MATCH_200");
  lua_pushinteger (L, MATCH_500);
  lua_setglobal (L, "faster.MATCH_500");
  lua_pushinteger (L, MATCH_401);
  lua_setglobal (L, "faster.MATCH_401");
  lua_pushinteger (L, MATCH_403);
  lua_setglobal (L, "faster.MATCH_403");
  lua_pushinteger (L, MATCH_404);
  lua_setglobal (L, "faster.MATCH_404");
  lua_pushinteger (L, MATCH_301);
  lua_setglobal (L, "faster.MATCH_301");
  lua_pushinteger (L, MATCH_301);
  lua_setglobal (L, "faster.MATCH_302");
  lua_pushinteger (L, MATCH_302);
  
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

void get_faster_value(luaState *L, char *n){
  lua_getglobal(L, "faster");
  lua_pushstring(L,n);
  lua_gettable(L,-2);
  lua_remove(L,-2);
}



void set_faster_value(luaState *L, char *n){
  lua_getglobal(L, "faster");
  lua_pushstring(L,n);
  lua_push(L,-3);
  lua_settable(L,-3);
  lua_pop();
  lua_pop();
}

static void target2lua(lua_State *L, struct target *t) {
  struct target **ptrptr=(struct target **) lua_newuserdata(L, sizeof(struct target *));
  *ptrptr=t;
  luaL_getmetatable(L, "faster.target");
  lua_setmetatable(L, -2);
}


int lua_callback_evaluate(struct http_request *req, struct http_response *rep, void *data){
  int rc;
  struct l_callback *callback= (struct l_callback*) data;
  lua_rawgeti(L, LUA_REGISTRYINDEX, callback->callback_ref);
  lua_rawgeti(L, LUA_REGISTRYINDEX, callback->data_ref);
  l_req_rep_2_lua(req,rep);
  lua_call(L, 2, 1);
  if(lua_isnil(L,-1)){
    lua_pop(L);
    return DETECT_NEXT_RULE;
  }
  rc=lua_toboolean(L,-1);
  lua_pop(L);
  return (rc ? DETECT_SUCCESS : DETECT_FAIL);
  
}


static struct match_rule *lua2match(lua_State *L) {
  match->size=-1;
  lua_getfield(L, 1, "code");
  if(!lua_isnil(L,-1)){
    if(!lua_isnumber(L,-1)){
      error="code must be a number";
      goto lua2match_error;
    }
    match->code=(int) lua_tointeger(L, -1);
  }

  lua_getfield(L, 1, "size");
  if(!lua_isnil(L,-1)){
    if(!lua_isnumber(L,-1)){
      error="size must be a number";
      goto lua2match_error;
    }
    match->size=(int) lua_tointeger(L, -1);
  }
  lua_pop();
  lua_getfield(L, 1, "method");
  if(!lua_isnil(L,-1)){
    if(!lua_isstring(L,-1)){
      error="method must be a string";
      goto lua2match_error;
    }
    match->method=strcpy(lua_tostring(L, -1));
  }
  lua_pop();
  lua_getfield(L, 1, "func");
  if(lua_isnil(L,-1)){
      error="func is required";
      goto lua2match_error;
  }

  lua_getfield(L, 1, "func_data");
  if(callback) callback->data_ref=luaL_ref(L, LUA_REGISTRYINDEX);
  lua_pop();
  return match;
}



}

int l_arb_setprop(luaState *L){
  lua_getfenv(L, 1);
  lua_pushvalue(L,2);
  lua_pushvalue(L,3);
  lua_settable(L,-3);
  return 0;
}



int l_arb_getprop(luaState *L){
  lua_getfenv(L, 1);
  lua_pushvalue(L,2);
  lua_gettable(L,-2);
  if(lua_isnil(L,-1)){
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

static int l_target_host(lua_State *L){
  struct target *t=check_target(L);
  lua_pushstring(L, t->host);
  return 1;
}



static void l_req_rep_2_lua(struct http_request *req, struct http_response *rep){
  lua_newtable(L);
  char *fullpath;
  char *path;
  lua_push_string(L,req->method);
  lua_setfield(L,-2,"method");

  path=serialize_method(req,0,0);
  lua_push_string(L,path);
  lua_setfield(L,-2,"path");
  ck_free(path);

  full_path=serialize_method(req,1,1);
  lua_push_string(L,full_path);
  lua_setfield(L,-2,"fullpath");
  ck_free(full_path);

  lua_push_lstring(L,res->payload,res->payload_len);
  lua_setfield(L,-2,"payload");

  lua_push_integer(L,res->code);
  lua_setfield(L,-2,"code");

  if(res->mime_type){
    lua_push_string(L,res->mime_type);
    lua_setfield(L,-2,"mime_type");
   }
}

void do_lua_callback(struct l_callback *callback, struct http_request *req, struct http_response *rep){
  lua_rawgeti(L, LUA_REGISTRYINDEX, callback->callback_ref);
  lua_rawgeti(L, LUA_REGISTRYINDEX, callback->data_ref);
  l_req_rep_2_lua(req,rep);
  lua_call(L, 2, 2);
}



void free_lua_callback(struct l_callback *callback){
  if(!callback) return;
  luaL_unref(L, LUA_REGISTRYINDEX, callback->callback_ref);
  luaL_unref(L, LUA_REGISTRYINDEX, callback->data_ref);
  ck_free(callback);
}

static int l_target_enqueue(lua_State *L){
  struct target *t=check_target(L);
  struct l_callback *on_success=NULL;
  char *path=luaL_checkstring(L,2);
  char *error="";
  http_request *req=new_request(t,path)
    
  if(lua_istable(L,3)){
     lua_getfield(L,3,"on_success");
     if(lua_isfunction(L,-1)){
      on_success=ck_malloc(sizeof(struct l_callback));
      on_success->callback_ref=luaL_ref(L, LUA_REGISTRYINDEX);
      on_success->data_ref=LUA_NILREF;
     }
     else{
      lua_pop(1);
     }
     
     lua_getfield(L,3,"callback_data");
     if(!lua_isnil(L,-1)){
       if(on_success) on_success->data_ref=luaL_ref(L, LUA_REGISTRYINDEX);
     }
     else{
       lua_pop(1);
     }
  }
  req->l_on_success=on_success;
  async_request(req);
  lua_pushboolean(L, 1);
  return 1;

l_target_enqueue_error:
  ck_free(req);
  lua_pushnil(L);
  lua_pushliteral(L,error);
  return 2;
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


