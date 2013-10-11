#include <memory>

#include "lua.hpp"
#include "conf0.h"

void main()
{
	std::shared_ptr<lua_State> L(luaL_newstate(), lua_close);
	luaL_openlibs(&*L);
	luaopen_conf0(&*L);
    if(luaL_loadfile(&*L, "test.lua") || lua_pcall(&*L, 0, 0, 0))    
		printf("ERROR: %s\n", lua_tostring(&*L, -1));
}
