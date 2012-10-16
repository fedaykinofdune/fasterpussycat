bin_PROGRAMS = fasterpussycat

AM_CXXFLAGS = $(IDN_CFLAGS) $(CRYPTO_CFLAGS) $(M_CFLAGS) $(MAGIC_CFLAGS) $(SQLITE3_CFLAGS) $(LUA_CFLAGS) $(SSL_CFLAGS) $(Z_CFLAGS)

BUILT_SOURCES = parser.h
AM_YFLAGS = -d


fasterpussycat_SOURCES = fasterpussycat.c tadns.c tadns.h \
  http_client.c http_client.h match_rule.c match_rule.h  ac.c ac.h string-inl.h alloc-inl.h \
  util.c util.h blacklist.c blacklist.h parser.y rule.l ast.c ast.h lua.h lua.c lua_http_client.h lua_http_client.c

fasterpussycat_LDADD = $(IDN_LIBS)  $(CRYPTO_LIBS) $(M_LIBS) $(MAGIC_LIBS) $(SQLITE3_LIBS) $(SSL_LIBS) $(Z_LIBS) $(LUA_LIBS)
