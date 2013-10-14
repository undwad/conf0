/*
** luaconf0.cpp by undwad
** crossplatform (bonjour (windows/osx), avahi(linux)) zeroconf(mdns) module for lua 5.2 
** https://github.com/undwad/conf0 mailto:undwad@mail.ru
** see copyright notice in conf0.h
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lua.hpp"

#include "conf0.h"
#include "luaconf0.h"

#if LUA_VERSION_NUM < 502
#  define luaL_newlib(L,l) (lua_newtable(L), luaL_register(L,NULL,l))
#endif

static int luaerror(lua_State *L, const char* text, int code)
{
	lua_pushnil(L);
	lua_pushstring(L, text);
	lua_pushinteger(L, code);
	return 3;
}

static int bytes(lua_State *L) 
{ 
	if(lua_isnumber(L, 1)) 
	{ 
		int value  = lua_tointeger(L, 1); 
		char result[sizeof(value)]; 
		memcpy(result, &value, sizeof(value)); 
		lua_pushlstring(L, result, sizeof(value)); 
		return 1; 
	} 
	return luaerror(L, "bytes functions works only on numeric types", 0);
}

#define luaSetField(L, index, name, value, proc) proc(L, value); lua_setfield(L, index, name);
#define luaSetBooleanField(L, index, name, value) luaSetField(L, index, name, value, lua_pushboolean);
#define luaSetNumberField(L, index, name, value) luaSetField(L, index, name, value, lua_pushnumber);
#define luaSetIntegerField(L, index, name, value) luaSetField(L, index, name, value, lua_pushinteger);
#define luaSetUnsignedField(L, index, name, value) luaSetField(L, index, name, value, lua_pushunsigned);
#define luaSetStringField(L, index, name, value) luaSetField(L, index, name, value, lua_pushstring);
#define luaSetCFunctionField(L, index, name, value) luaSetField(L, index, name, value, lua_pushcfunction);
#define luaSetUserDataField(L, index, name, value) luaSetField(L, index, name, value, lua_pushlightuserdata);
#define luaSetLStringField(L, index, name, value, len) lua_pushlstring(L, (const char*)value, len); lua_setfield(L, index, name);

#define luaParam(L, index, type, name, isproc, toproc) \
	if(!isproc(L, index)) return luaerror(L, "parameter "#name##" must be a valid "#type, index);  \
	name = toproc(L, index); 

#define luaMandatoryParam(L, index, type, name, isproc, toproc) \
	type name; \
	luaParam(L, index, type, name, isproc, toproc);

#define luaMandatoryBooleanParam(L, index, name) luaMandatoryParam(L, index, bool, name, lua_isnumber, lua_toboolean)
#define luaMandatoryNumberParam(L, index, name) luaMandatoryParam(L, index, float, name, lua_isnumber, lua_tonumber)
#define luaMandatoryIntegerParam(L, index, name) luaMandatoryParam(L, index, int, name, lua_isnumber, lua_tointeger)
#define luaMandatoryUnsignedParam(L, index, name) luaMandatoryParam(L, index, unsigned int, name, lua_isnumber, lua_tounsigned)
#define luaMandatoryStringParam(L, index, name) luaMandatoryParam(L, index, const char*, name, lua_isstring, lua_tostring)
#define luaMandatoryUserDataParam(L, index, name) luaMandatoryParam(L, index, void*, name, lua_isuserdata, lua_touserdata)
#define luaMandatoryCallbackParam(L, index, name) \
	if(!lua_isfunction(L, index)) return luaerror(L, "parameter "#name##" must be a valid function", index); \
	lua_pushvalue(L, index); \
	int name = luaL_ref(L, LUA_REGISTRYINDEX);

#define luaOptionalParam(L, index, type, name, value, isproc, toproc) \
	type name = value; \
	if(!lua_isnoneornil(L, index)) \
	{ \
		luaParam(L, index, type, name, isproc, toproc); \
	}

#define luaOptionalBooleanParam(L, index, name, value) luaOptionalParam(L, index, bool, name, value, lua_isnumber, lua_toboolean)
#define luaOptionalNumberParam(L, index, name, value) luaOptionalParam(L, index, float, name, value, lua_isnumber, lua_tonumber)
#define luaOptionalIntegerParam(L, index, name, value) luaOptionalParam(L, index, int, name, value, lua_isnumber, lua_tointeger)
#define luaOptionalUnsignedParam(L, index, name, value) luaOptionalParam(L, index, unsigned int, name, value, lua_isnumber, lua_tounsigned)
#define luaOptionalStringParam(L, index, name, value) luaOptionalParam(L, index, const char*, name, value, lua_isstring, lua_tostring)
#define luaOptionalUserDataParam(L, index, name, value) luaOptionalParam(L, index, void*, name, value, lua_isuserdata, lua_touserdata)

struct userdata_t
{
	lua_State* L;
	int index;

	userdata_t(lua_State *l, int i) : L(l), index(i) { }

	~userdata_t()
	{
		if(LUA_NOREF != index) 
			luaL_unref(L, LUA_REGISTRYINDEX, index); 
	}
};

struct ctx_t
{
	void* context;
	userdata_t* userdata;
};

static int gc(lua_State *L) 
{
	luaMandatoryUserDataParam(L, 1, context);
	typedef void (*proc_t)(void* context);
	proc_t proc = (proc_t)lua_touserdata(L, lua_upvalueindex(1));
	ctx_t* ctx = (ctx_t*)context;
	proc(ctx->context);
	delete ctx->userdata;
	return 0;
}

#define beginNewContext(name) \
	static int name(lua_State *L) \
	{ \


#define endNewContext(ctor, dctor, idx, ...) \
		userdata_t* userdata = new userdata_t(L, idx); \
		void* context = ctor(__VA_ARGS__); \
		if(context) \
		{ \
			ctx_t* ctx = (ctx_t*) lua_newuserdata(L, sizeof(ctx_t)); \
			ctx->context = context; \
			ctx->userdata = userdata; \
			lua_newtable(L); \
			lua_pushlightuserdata(L, dctor); \
			lua_pushcclosure(L, gc, 1); \
			lua_setfield(L, -2, "__gc"); \
			lua_setmetatable(L, -2); \
			return 1; \
		} \
		else \
		{ \
			delete userdata; \
			return luaerror(L, conf0_error_text(), conf0_error_code()); \
		} \
	}

#define beginReplyCallback(name, ...) \
	void name(void* context, unsigned int flags, unsigned int interface_, bool error, __VA_ARGS__, void* userdata) \
	{ \
		userdata_t* userdata_ = (userdata_t*)userdata; \
		lua_State *L = userdata_->L; \
		lua_rawgeti(L, LUA_REGISTRYINDEX, userdata_->index); \
		lua_newtable(L); \
		luaSetUnsignedField(L, -2, "flags", flags); \
		luaSetUnsignedField(L, -2, "interface", interface_); \
		if(error) \
		{ \
			lua_newtable(L); \
			luaSetStringField(L, -2, "text", conf0_error_text()); \
			luaSetIntegerField(L, -2, "code", conf0_error_code()); \
			lua_setfield(L, -2, "error"); \
		} \

#define endReplyCallback(name) \
		if(lua_pcall(L, 1, 0, 0)) \
		{ \
			fprintf(stderr, #name##" callback error %s\n", lua_tostring(L, -1)); \
			lua_pop(L, 1); \
		}	\
	}

/* COMMON */

beginNewContext(common)
endNewContext(conf0_common_alloc, conf0_common_free, LUA_NOREF)

/* DOMAIN */

extern const int browser_browse;

beginReplyCallback(enumdomain_callback, const char* domain)
	luaSetStringField(L, -2, "domain", domain)
endReplyCallback(enumdomain_callback)

beginNewContext(enumdomain)
	luaMandatoryUserDataParam(L, 1, common_context)
	luaMandatoryCallbackParam(L, 2, callback) 
	luaOptionalUnsignedParam(L, 3, flags, browser_browse) 
	luaOptionalUnsignedParam(L, 4, interface_, 0) 
endNewContext(conf0_enumdomain_alloc, conf0_enumdomain_free, callback, ((ctx_t*)common_context)->context, flags, interface_, enumdomain_callback, userdata)

/* BROWSER */

beginReplyCallback(browser_callback, const char* name, const char* type, const char* domain)
	luaSetStringField(L, -2, "name", name)
	luaSetStringField(L, -2, "type", type)
	luaSetStringField(L, -2, "domain", domain)
endReplyCallback(browser_callback)

beginNewContext(browse)
	luaMandatoryUserDataParam(L, 1, common_context)
	luaMandatoryStringParam(L, 2, type)
	luaMandatoryCallbackParam(L, 3, callback) 
	luaOptionalStringParam(L, 4, domain, "")
	luaOptionalUnsignedParam(L, 5, flags, 0) 
	luaOptionalUnsignedParam(L, 6, interface_, 0) 
endNewContext(conf0_browser_alloc, conf0_browser_free, callback, ((ctx_t*)common_context)->context, flags, interface_, type, domain, browser_callback, userdata)

/* RESOLVER */

beginReplyCallback(resolver_callback, const char* fullname, const char* hosttarget, unsigned short opaqueport, unsigned short textlen, const unsigned char* text)
	luaSetStringField(L, -2, "fullname", fullname);
	luaSetStringField(L, -2, "hosttarget", hosttarget);
	luaSetUnsignedField(L, -2, "opaqueport", opaqueport)
	luaSetLStringField(L, -2, "text", text, textlen);
endReplyCallback(resolver_callback)

beginNewContext(resolve)
	luaMandatoryUserDataParam(L, 1, common_context)
	luaMandatoryStringParam(L, 2, name) 
	luaMandatoryStringParam(L, 3, type) 
	luaMandatoryStringParam(L, 4, domain) 
	luaMandatoryCallbackParam(L, 5, callback) 
	luaOptionalUnsignedParam(L, 6, flags, 0) 
	luaOptionalUnsignedParam(L, 7, interface_, 0) 
endNewContext(conf0_resolver_alloc, conf0_resolver_free, callback, ((ctx_t*)common_context)->context, flags, interface_, name, type, domain, resolver_callback, userdata)

/* QUERY */

extern const int record_class;

beginReplyCallback(query_callback, const char* fullname, unsigned short type, unsigned short class_, unsigned short datalen, const void* data, unsigned int ttl)
	luaSetStringField(L, -2, "fullname", fullname);
	luaSetUnsignedField(L, -2, "type", type);
	luaSetUnsignedField(L, -2, "class", class_);
	luaSetLStringField(L, -2, "data", data, datalen);
	luaSetUnsignedField(L, -2, "ttl", ttl);
endReplyCallback(query_callback)

beginNewContext(query)
	luaMandatoryUserDataParam(L, 1, common_context)
	luaMandatoryStringParam(L, 2, fullname) 
	luaMandatoryUnsignedParam(L, 3, type) 
	luaMandatoryCallbackParam(L, 4, callback) 
	luaOptionalUnsignedParam(L, 5, class_, record_class) 
	luaOptionalUnsignedParam(L, 6, flags, 0) 
	luaOptionalUnsignedParam(L, 7, interface_, 0) 
endNewContext(conf0_query_alloc, conf0_query_free, callback, ((ctx_t*)common_context)->context, flags, interface_, fullname, type, class_, query_callback, userdata)

/* REGISTER */

beginReplyCallback(register_callback, const char* name, const char* type, const char* domain)
	luaSetStringField(L, -2, "name", name)
	luaSetStringField(L, -2, "type", type)
	luaSetStringField(L, -2, "domain", domain)
endReplyCallback(register_callback)

beginNewContext(register_)
	luaMandatoryUserDataParam(L, 1, common_context)
	luaMandatoryStringParam(L, 2, type);
	luaMandatoryCallbackParam(L, 3, callback) 
	luaOptionalStringParam(L, 4, name, nullptr);
	luaOptionalStringParam(L, 5, host, nullptr);
	luaOptionalUnsignedParam(L, 6, port, 0);
	luaOptionalStringParam(L, 7, domain, nullptr);
	luaOptionalUnsignedParam(L, 8, interface_, 0);
	luaOptionalUnsignedParam(L, 9, flags, 0);
	luaOptionalStringParam(L, 10, text, nullptr);
	luaOptionalUnsignedParam(L, 11, textlen, 0);
endNewContext(conf0_register_alloc, conf0_register_free, callback, ((ctx_t*)common_context)->context, flags, interface_, name, type, domain, host, port, textlen, text, register_callback, userdata)

static int iterate(lua_State *L) 
{
	luaMandatoryUserDataParam(L, 1, context);
	luaOptionalUnsignedParam(L, 2, timeout, 5000);
	int result = conf0_iterate(((ctx_t*)context)->context, timeout);
	if(result < 0)
		return luaerror(L, conf0_error_text(), conf0_error_code());
	else if(result > 0)
	{
		lua_pushboolean(L, true);
		return 1;
	}
	lua_pushboolean(L, false);
	return 1;
}

static const struct luaL_Reg lib[] = 
{
	{"bytes", bytes},
	{"common", common},
	{"enumdomain", enumdomain},
	{"browse", browse},
	{"resolve", resolve},
	{"query", query},
	{"register", register_},
	{"iterate", iterate},
    {nullptr, nullptr},
};

int luaopen_conf0(lua_State *L) 
{
    luaL_newlib (L, lib);
    lua_setglobal(L, "conf0");
	return 1;
}
