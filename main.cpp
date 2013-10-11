//#include "../inc/sysdefs.h"

#include <iostream>
#include <string>
#include <memory>

#include "lua/lua.hpp"
#include "conf0.h"

using namespace std;

void main()
{
	shared_ptr<lua_State> L(luaL_newstate(), lua_close);
	luaL_openlibs(&*L);
	luaopen_conf0(&*L);
    if(luaL_loadfile(&*L, "test.lua") || lua_pcall(&*L, 0, 0, 0))    
		printf("ERROR: %s\n", lua_tostring(&*L, -1));
}
