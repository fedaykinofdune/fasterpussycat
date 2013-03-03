#include "lua_helpers.h"

int main(int argc, char **argv){
  lua_initialize();
  if(luaL_dofile (l_state, "./lua/init.lua")){
    printf("%s\n", lua_tostring(l_state, -1));     
  }

}
