/*
** luamisc.h by undwad
** macro extensions for lua 5.2 api
** mailto:undwad@mail.ru
** see copyright notice at the end of this file
*/

#ifndef _LUA_MISC_H__
#define _LUA_MISC_H__

#if !defined(nullptr)
#	define nullptr NULL
#endif

#include "lua.hpp"

#define luaM_function_begin(NAME) \
	static int NAME(lua_State* L) \
	{ \
		int result = 0; \
		if(!lua_istable(L, 1)) \
			return luaL_error(L, "a single function parameter must be of type table");

#define luaM_return(TYPE, ...) \
	lua_push##TYPE(L, __VA_ARGS__); \
	result++;

#define luaM_function_end \
		return result; \
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

