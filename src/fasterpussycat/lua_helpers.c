void register_lua_faster_funcs(lua_State *L, char *name, const luaL_reg *funcs){
  luaL_register(L, name, funcs);
}

