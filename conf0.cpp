/*
** conf0.cpp by undwad
** crossplatform (bonjour (windows/osx), avahi(linux)) zeroconf(mdns) module for lua 5.2
** https://github.com/undwad/conf0 mailto:undwad@mail.ru
** see copyright notice at the end of this file
*/

#if defined( __SYMBIAN32__ )
#	define SYMBIAN
#elif defined( __WIN32__ ) || defined( _WIN32 ) || defined( WIN32 )
#	ifndef WIN32
#		define WIN32
#	endif
#elif defined( __APPLE_CC__)
#   if __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__ >= 40000 || __IPHONE_OS_VERSION_MIN_REQUIRED >= 40000
#		define IOS
#   else
#		define OSX
#   endif
#elif defined(linux) && defined(__arm__)
#	define TEGRA2
#elif defined(__ANDROID__)
#	define ANDROID
#elif defined( __native_client__ )
#	define NATIVECLIENT
#else
#	define LINUX
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "luamisc.h"


#if defined(WIN32)
#	include "dns_sd.h"
#	pragma comment(lib, "dnssd.lib")
#	pragma comment(lib, "ws2_32.lib")

luaM_func_begin(test)
	luaM_reqd_param(boolean, p1)
	luaM_reqd_param(number, p2)
	luaM_reqd_param(integer, p3)
	luaM_reqd_param(unsigned, p4)
	luaM_reqd_param(string, p5)
	luaM_reqd_param(table, p6)
	luaM_reqd_param(function, p7)
	printf("%d %f %d %d %s %d %d\n",p1,p2,p3,p4,p5,p6,p7);
	luaM_return(boolean, true)
	luaM_return(integer, 123)
	luaM_return(number, 123.321)
	luaM_return(lightuserdata, nullptr)
	luaM_return(string, "JODER")
	luaM_return(literal, "JODER")
	luaM_return(nil)
luaM_func_end


#elif defined(LINUX)

#   include <avahi-client/client.h>
#   include <avahi-client/lookup.h>

#   include <avahi-common/simple-watch.h>
#   include <avahi-common/malloc.h>
#   include <avahi-common/error.h>


#elif defined(OSX)
#	error incompatible platform
#else
#	error incompatible platform
#endif

static const struct luaL_Reg lib[] = 
{
	{"test", test},
	//{"common", common},
	//{"enumdomain", enumdomain},
	//{"browse", browse},
	//{"resolve", resolve},
	//{"query", query},
	//{"register", register_},
	//{"iterate", iterate},
    {nullptr, nullptr},
};

extern "C"
{
	LUAMOD_API int luaopen_conf0(lua_State *L) 
	{
		luaL_newlib (L, lib);
		lua_setglobal(L, "conf0");
		return 1;
	}
}

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
