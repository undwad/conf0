/*
** luaM.h by undwad
** macro extensions for lua 5.2 C api
** mailto:undwad@mail.ru
** see copyright notice at the end of this file
*/

#ifndef _LUA_MISC_H__
#define _LUA_MISC_H__

#if !defined(nullptr)
#	define nullptr NULL
#endif

#include "lua.hpp"

#define luaM__gc(TYPE) \
	static int TYPE##__gc(lua_State *L) \
	{ \
		if(!lua_isuserdata(L, 1)) \
			return luaL_error(L, "single __gc parameter must be of type userdata"); \
		TYPE* ptr = (TYPE*)lua_touserdata(L, 1); \
		ptr->TYPE::~TYPE(); \
		return 0; \
	}

#define luaM_func_begin(NAME) \
	static int NAME(lua_State* L) \
	{ \
		int result = 0; \
		if(!lua_istable(L, 1)) \
			return luaL_error(L, "single function parameter must be of type table");

#define lua_isinteger lua_isnumber
#define lua_isunsigned lua_isnumber

static int lua_toregistry(lua_State* L, int idx)
{
	lua_pushvalue(L, idx); 
	return luaL_ref(L, LUA_REGISTRYINDEX);
}

#define lua_tofunction(L, IDX) lua_toregistry(L, IDX)
#define lua_totable(L, IDX) lua_toregistry(L, IDX)

#define luaM_type_boolean bool
#define luaM_type_number double
#define luaM_type_integer int
#define luaM_type_unsigned unsigned int
#define luaM_type_string const char*
#define luaM_type_userdata void*
#define luaM_type_cfunction lua_CFunction
#define luaM_type_function int
#define luaM_type_table int

#define luaM_reqd_param(TYPE, NAME) \
	lua_getfield(L, 1, #NAME); \
	if(!lua_is##TYPE(L, -1)) \
	{ \
		lua_pop(L, 1); \
		return luaL_error(L, "required parameter '" #NAME "' must be of type " #TYPE); \
	} \
	luaM_type_##TYPE NAME = lua_to##TYPE(L, -1); \
	lua_pop(L, 1); 

#define luaM_opt_param(TYPE, NAME, DEF) \
	luaM_type_##TYPE NAME = DEF; \
	lua_getfield(L, 1, #NAME); \
	if(lua_is##TYPE(L, -1)) \
		NAME = lua_to##TYPE(L, -1); \
	lua_pop(L, 1); 

#define luaM_return(TYPE, ...) \
	lua_push##TYPE(L, __VA_ARGS__); \
	result++;

#define luaM_return_userdata(TYPE, INIT, NAME, ...) \
	TYPE* NAME = (TYPE*)lua_newuserdata(L, sizeof(TYPE)); \
	NAME->TYPE::INIT(__VA_ARGS__); \
	lua_newtable(L); \
	lua_pushcfunction(L, TYPE##__gc); \
	lua_setfield(L, -2, "__gc"); \
	lua_setmetatable(L, -2); \
	result++;

#define luaM_func_end \
		return result; \
	}

#define luaM_setfield(IDX, TYPE, NAME, ...) \
	lua_push##TYPE(L, __VA_ARGS__); \
	lua_setfield(L, IDX < 0 ? IDX - 1 : IDX, #NAME);

static void luaM_print_value(lua_State* L, FILE* f, int i = -1)
{
	switch (lua_type(L, i))
	{
	case LUA_TNIL: fprintf(f, "%s nil\n", luaL_typename(L, i)); break;
	case LUA_TBOOLEAN: fprintf(f, "%s %s\n", luaL_typename(L, i), lua_toboolean(L, i) ? "true" : "false"); break;
	case LUA_TNUMBER: fprintf(f, "%s %f\n", luaL_typename(L, i), lua_tonumber(L, i)); break;
	case LUA_TSTRING: fprintf(f, "%s %s\n", luaL_typename(L, i), lua_tostring(L, i)); break;
	case LUA_TTABLE:
	case LUA_TFUNCTION:
	case LUA_TTHREAD:
	case LUA_TLIGHTUSERDATA:
	case LUA_TUSERDATA: fprintf(f, "%s %x\n", luaL_typename(L, i), lua_topointer(L, i)); break;
	}
}

static void luaM_print_stack(lua_State* L, FILE* f)
{
	for (int i = 1; i <= lua_gettop(L); i++)
	{
		fprintf(f, "%d: ", i);
		luaM_print_value(L, f, i);
	}
}

static void luaM_save_stack(lua_State* L, const char* path)
{ 
	FILE* f = fopen(path, "w");
	luaM_print_stack(L, f); 
	fflush(f);
	fclose(f);
}

static int luaM_save_stack(lua_State* L)
{ 
	const char* path = lua_tostring(L, 1);
	if(path)
	{
		lua_pop(L, 1);
		luaM_save_stack(L, path);
		return 0;
	}
	return luaL_error(L, "invalid output file path");
}

#endif // _LUA_MISC_H__

/******************************************************************************
* Copyright (C) 2013 Undwad, Samara, Russia
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/

