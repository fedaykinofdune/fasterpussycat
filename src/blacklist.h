#ifndef FASTERPUSSYCAT_BLACKLIST_H
#define FASTERPUSSYCAT_BLACKLIST_H

#include "lua.h"
#include "http_client.h"

/* blacklist.c */
void register_blacklist(void);
int l_is_response_blacklisted(lua_State *L);
int l_blacklist_response(lua_State *L);
struct blacklist *check_blacklist(lua_State *L, int pos);
int l_new_blacklist(lua_State *L);

struct blacklist;

struct blacklist {
  int start;
  struct blacklist *next;
  struct http_sig sig;
};

#endif
