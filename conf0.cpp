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

#include "luaM.h"

luaM_func_begin(test)
	luaM_reqd_param(boolean, p1)
	luaM_reqd_param(number, p2)
	luaM_reqd_param(integer, p3)
	luaM_reqd_param(unsigned, p4)
	luaM_reqd_param(string, p5)
	luaM_reqd_param(table, p6)
	luaM_reqd_param(function, p7)
	printf("%d %f %d %d %s %d %d\n",p1,p2,p3,p4,p5,p6,p7);
	luaM_opt_param(boolean, p8, false)
	luaM_opt_param(number, p9, 2.34)
	luaM_opt_param(integer, p10, -10)
	luaM_opt_param(unsigned, p11, 10)
	luaM_opt_param(string, p12, "default")
	printf("%d %f %d %d %s\n",p8,p9,p10,p11,p12);
	luaM_return(boolean, true)
	luaM_return(integer, 123)
	luaM_return(number, 123.321)
	luaM_return(lightuserdata, nullptr)
	luaM_return(string, "JODER")
	luaM_return(literal, "JODER")
	luaM_return(nil)
luaM_func_end

#if defined(WIN32)
#	include "dns_sd.h"
#	pragma comment(lib, "dnssd.lib")
#	pragma comment(lib, "ws2_32.lib")

#	define conf0_callback_begin(CALLBACK, ...) \
	void DNSSD_API CALLBACK(DNSServiceRef ref, DNSServiceFlags flags, __VA_ARGS__, void* userdata) \
	{ \
		context_t* context = (context_t*)userdata; \
		lua_State *L = context->L; \
		lua_rawgeti(L, LUA_REGISTRYINDEX, context->callback); \
		lua_newtable(L); \
		luaM_setfield(-1, unsigned, flags, flags)

#	define conf0_callback_end(CALLBACK) \
		if(lua_pcall(L, 1, 0, 0)) \
		{ \
			fprintf(stderr, #CALLBACK " error %s\n", lua_tostring(L, -1)); \
			lua_pop(L, 1); \
		} \
	}

# define conf0_call_dns_service(FUNC, ...) \
	int error = FUNC(__VA_ARGS__); \
	if(kDNSServiceErr_NoError != error) \
	{ \
		delete context; \
		return luaL_error(L, #FUNC "() failed with error %d", error); \
	}

//#	define BEGINFUNC(FUNC, CALLBACK, ...) \
//	void* FUNC(void* common_context, unsigned int flags, unsigned int interface_, __VA_ARGS__, CALLBACK callback, void* userdata) \
//	{
//
//#	define ENDFUNC(FUNC, ...) \
//		DNSServiceRef ref = nullptr; \
//		userdata_t* userdata_ = new userdata_t(callback, userdata); \
//		DNSServiceErrorType error = FUNC(&userdata_->ref, (DNSServiceFlags)flags, interface_, __VA_ARGS__, userdata_); \
//		if(kDNSServiceErr_NoError == error) return userdata_; \
//		set_error(#FUNC "() failed", error); \
//		delete userdata_; \
//		return nullptr; \
//	}

	/* CONNECT */

	struct context_t
	{
		lua_State* L;
		int callback;
		DNSServiceRef ref;
		context_t(lua_State* L_, int callback_) : L(L_), callback(callback_), ref(nullptr) { }
		~context_t() 
		{ 
			if(LUA_NOREF != callback)
				luaL_unref(L, LUA_REGISTRYINDEX, callback);
			if(ref)
				DNSServiceRefDeallocate(ref); 
		}
	};

	luaM__gc(context_t)

	luaM_func_begin(iterate)
		luaM_reqd_param(userdata, ref)
		luaM_opt_param(integer, timeout, 5000)
		context_t* context = (context_t*)ref;
		int dns_sd_fd  = DNSServiceRefSockFD(context->ref);
		int nfds = dns_sd_fd + 1;
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(dns_sd_fd , &readfds);
		timeval tv;
		tv.tv_sec  = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;
		int res = select(nfds, &readfds, (fd_set*)NULL, (fd_set*)NULL, &tv);
		if (res > 0)
		{
			DNSServiceErrorType error = kDNSServiceErr_NoError;
			if(FD_ISSET(dns_sd_fd , &readfds))
				error = DNSServiceProcessResult(context->ref);
			if (kDNSServiceErr_NoError == error)
			{
				luaM_return(boolean, true)
			}
			else
				return luaL_error(L, "DNSServiceProcessResult() failed with error", error);
		}
		else if(res < 0) //EINTR???
			return luaL_error(L, "select() failed with error", errno);
		else
			luaM_return(boolean, false)
	luaM_func_end

	/* BROWSE */

	conf0_callback_begin(browse_callback, uint32_t interface_, DNSServiceErrorType error, const char* name, const char* type, const char* domain)
		luaM_setfield(-1, unsigned, interface, interface_)
		luaM_setfield(-1, string, name, name)
		luaM_setfield(-1, string, type, type)
		luaM_setfield(-1, string, domain, domain)
	conf0_callback_end(browse_callback)

	luaM_func_begin(browse)
		luaM_opt_param(unsigned, flags, 0)
		luaM_opt_param(unsigned, interface_, 0)
		luaM_reqd_param(string, type)
		luaM_opt_param(string, domain, nullptr)
		luaM_reqd_param(function, callback)
		luaM_return_userdata(context_t, context, L, callback)
		conf0_call_dns_service(DNSServiceBrowse, &context->ref, (DNSServiceFlags)flags, interface_, type, domain, browse_callback, context)
	luaM_func_end

	/* RESOLVE */

	conf0_callback_begin(resolve_callback, uint32_t interface_, DNSServiceErrorType error, const char* fullname, const char* hosttarget, uint16_t opaqueport, uint16_t textlen, const unsigned char* text)
		luaM_setfield(-1, unsigned, interface, interface_)
		luaM_setfield(-1, string, fullname, fullname)
		luaM_setfield(-1, string, hosttarget, hosttarget)
		luaM_setfield(-1, unsigned, opaqueport, opaqueport)
		luaM_setfield(-1, unsigned, textlen, textlen)
		luaM_setfield(-1, string, text, (const char*)text)
	conf0_callback_end(resolve_callback)

	luaM_func_begin(resolve)
		luaM_opt_param(unsigned, flags, 0)
		luaM_opt_param(unsigned, interface_, 0)
		luaM_reqd_param(string, name)
		luaM_reqd_param(string, type)
		luaM_reqd_param(string, domain)
		luaM_reqd_param(function, callback)
		luaM_return_userdata(context_t, context, L, callback)
		conf0_call_dns_service(DNSServiceResolve, &context->ref, (DNSServiceFlags)flags, interface_, name, type, domain, resolve_callback, context)
	luaM_func_end

	/* QUERY */

	extern const int record_class = kDNSServiceClass_IN;

	void DNSSD_API query_callback(DNSServiceRef ref, DNSServiceFlags flags, uint32_t interface_, DNSServiceErrorType error, const char* fullname, uint16_t type, uint16_t class_, uint16_t datalen, const void* data, uint32_t ttl, void* userdata) 
	{
	}

	//BEGINFUNC(conf0_query_alloc, conf0_query_callback, const char* fullname, unsigned short type, unsigned short class_)
	//ENDFUNC(DNSServiceQueryRecord, fullname, type, class_, query_callback)

	//FREEPROC(conf0_query_free)

	/* REGISTER */

	void DNSSD_API register_callback(DNSServiceRef ref, DNSServiceFlags flags, DNSServiceErrorType error, const char* name, const char* type, const char* domain, void* userdata)
	{
	}

	//BEGINFUNC(conf0_register_alloc, conf0_register_callback, const char* name, const char* type, const char* domain, const char* host, unsigned short port, unsigned short textlen, const void* text)
	//ENDFUNC(DNSServiceRegister, name, type, domain, host, port, textlen, text, register_callback)

	//FREEPROC(conf0_register_free)

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
	{"savestack", luaM_save_stack},
	{"browse", browse},
	{"resolve", resolve},
	//{"query", query},
	//{"register", register_},
	{"iterate", iterate},
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
