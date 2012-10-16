#include "blacklist.h"
#include "lua_http_client.h"

void register_blacklist(){
  /* register target */

  static const struct luaL_reg blacklist_m [] = {
    {"__index", l_arb_getprop},
    {"__newindex", l_arb_setprop},
    
    {"new", l_new_blacklist},
    {"blacklist_response", l_blacklist_response},
    {"is_response_blacklisted", l_is_response_blacklisted},
    {NULL, NULL}
  };
  register_faster_funcs("blacklist",blacklist_m);



}

int l_is_response_blacklisted(lua_State *L){
  struct blacklist *b=check_blacklist(L, 1);
  if(b->next==NULL){
    lua_pushboolean(L,0);
    return 1;
  }
  struct http_response *res=check_http_response(L, 2);
  if(!res->fprinted) fprint_response(res); 
  struct blacklist *current;
  for(current=b->next;current->next;current=current->next){
    if(same_page(&res->sig, &current->sig)){
      lua_pushboolean(L,1);
      return 1;
    }
  }
  
  lua_pushboolean(L,0);
  return 1;
}



int l_blacklist_response(lua_State *L){
  struct blacklist *b=check_blacklist(L, 1);
  struct http_response *res=check_http_response(L, 2);
  if(!res->fprinted) fprint_response(res); 
  struct blacklist *old_top=b->next;
  b->next=calloc(sizeof(struct blacklist), 1);
  b->next->sig=res->sig;
  b->next->next=old_top;
  return 0;
}


struct blacklist *check_blacklist(lua_State *L, int pos){
   void *ud = luaL_checkudata(L, pos, "faster.blacklist");
   luaL_argcheck(L, ud != NULL, pos, "`faster.blacklist' expected");
   return ((struct blacklist *) ud);
}


int l_new_blacklist(lua_State *L){
  struct blacklist *b=lua_newuserdata(L, sizeof(struct blacklist));
  b->start=1;
  b->next=NULL;
  setup_obj_env();
  get_faster_value(L, "blacklist");
  lua_setmetatable(L, -2);
  return 1;
}

