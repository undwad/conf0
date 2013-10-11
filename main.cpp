//#include "../inc/sysdefs.h"

#include <iostream>
#include <string>
#include <memory>

using namespace std;

#include "dns_sd.h"

#include "lua/lua.hpp"

#pragma comment(lib, "dnssd.lib")
#pragma comment(lib, "ws2_32.lib")

int luaerror(lua_State *L, const char* text, int code = 0)
{
	lua_pushnil(L);
	lua_pushstring(L, text);
	lua_pushnumber(L, code);
	return 3;
}

#define luaSetField(L, index, name, value, proc) proc(L, value); lua_setfield(L, index, name)
#define luaSetBooleanField(L, index, name, value) luaSetField(L, index, name, value, lua_pushboolean)
#define luaSetNumberField(L, index, name, value) luaSetField(L, index, name, value, lua_pushnumber)
#define luaSetIntegerField(L, index, name, value) luaSetField(L, index, name, value, lua_pushinteger)
#define luaSetUnsignedField(L, index, name, value) luaSetField(L, index, name, value, lua_pushunsigned)
#define luaSetStringField(L, index, name, value) luaSetField(L, index, name, value, lua_pushstring)
#define luaSetCFunctionField(L, index, name, value) luaSetField(L, index, name, value, lua_pushcfunction)
#define luaSetUserDataField(L, index, name, value) luaSetField(L, index, name, value, lua_pushlightuserdata)
#define luaSetLStringField(L, index, name, value, len) lua_pushlstring(L, (const char*)value, len); lua_setfield(L, index, name)

#define luaParam(L, index, type, name, isproc, toproc) \
	if(!isproc(L, index)) return luaerror(L, "parameter "#name##" must be a valid "#type, index); \
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
	Context* name = new Context(L, luaL_ref(L, LUA_REGISTRYINDEX));

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

#define beginReplyCallback() \
	Context* ctx = (Context*)context; \
	lua_State *L = ctx->L; \
	lua_rawgeti(L, LUA_REGISTRYINDEX, ctx->callback); \
	lua_newtable(L); 

#define endReplyCallback(name) \
	if(lua_pcall(L, 1, 0, 0)) \
	{ \
		fprintf(stderr, name##" callback error %s\n", lua_tostring(L, -1)); \
		lua_pop(L, 1); \
	}	

#define requestError kDNSServiceErr_NoError

#define returnContext(name) \
	if(requestError == error) \
	{ \
		lua_pushlightuserdata(L, ctx); \
		return 1; \
	} \
	else \
	{ \
		delete ctx; \
		return luaerror(L, name##" failed", error); \
	}

struct conf0
{
	struct Context
	{
		lua_State *L;
		DNSServiceRef client;
		int callback;

		Context(lua_State *l, int c) : L(l), callback(c), client(nullptr) { }

		~Context() 
		{
			if (LUA_NOREF != callback) 
				luaL_unref(L, LUA_REGISTRYINDEX, callback); 
			if(client)
				DNSServiceRefDeallocate(client);
		}
	};

	static void DNSSD_API browseReply(DNSServiceRef client, const DNSServiceFlags flags, uint32_t _interface, DNSServiceErrorType error, const char* name, const char* type, const char* domain, void* context)
	{
		beginReplyCallback();
		luaSetUnsignedField(L, -2, "flags", flags);
		luaSetUnsignedField(L, -2, "interface", _interface);
		luaSetIntegerField(L, -2, "error", error);
		luaSetStringField(L, -2, "name", name);
		luaSetStringField(L, -2, "type", type);
		luaSetStringField(L, -2, "domain", domain);
		endReplyCallback("browse reply");
	}

	static int browse(lua_State *L) 
	{
		luaMandatoryCallbackParam(L, 1, ctx);
		luaOptionalStringParam(L, 2, type, "");
		luaOptionalStringParam(L, 3, domain, "");
		luaOptionalUnsignedParam(L, 4, _interface, 0);
		luaOptionalUnsignedParam(L, 5, flags, 0);
		int error = DNSServiceBrowse(&ctx->client, flags, _interface, type, domain, browseReply, ctx);
		returnContext("browse request");
	}

	static void DNSSD_API resolveReply(DNSServiceRef client, const DNSServiceFlags flags, uint32_t _interface, DNSServiceErrorType error,	const char* fullname, const char* hosttarget, uint16_t opaqueport, uint16_t textlen, const unsigned char* text, void* context)
	{
		beginReplyCallback();
		luaSetUnsignedField(L, -2, "flags", flags);
		luaSetUnsignedField(L, -2, "interface", _interface);
		luaSetIntegerField(L, -2, "error", error);
		luaSetStringField(L, -2, "fullname", fullname);
		luaSetStringField(L, -2, "hosttarget", hosttarget);
		luaSetUnsignedField(L, -2, "opaqueport", opaqueport);
		union { uint16_t s; u_char b[2]; } port = { opaqueport };
		luaSetUnsignedField(L, -2, "port", ((uint16_t)port.b[0]) << 8 | port.b[1]);
		luaSetLStringField(L, -2, "text", text, textlen);
		endReplyCallback("resolve reply");
	}

	static int resolve(lua_State *L) 
	{
		luaMandatoryStringParam(L, 1, name);
		luaMandatoryStringParam(L, 2, type);
		luaMandatoryStringParam(L, 3, domain);
		luaMandatoryCallbackParam(L, 4, ctx);
		luaOptionalUnsignedParam(L, 5, _interface, 0);
		luaOptionalUnsignedParam(L, 6, flags, 0);
		int error = DNSServiceResolve(&ctx->client, flags, _interface, name, type, domain, resolveReply, ctx);
		returnContext("resolve request");
	}

	static void DNSSD_API queryReply(DNSServiceRef client, const DNSServiceFlags flags, uint32_t _interface, DNSServiceErrorType error,	const char* fullname, uint16_t recordtype, uint16_t recordclass, uint16_t datalen, const void* data, uint32_t ttl, void* context)
	{
		beginReplyCallback();
		luaSetUnsignedField(L, -2, "flags", flags);
		luaSetUnsignedField(L, -2, "interface", _interface);
		luaSetIntegerField(L, -2, "error", error);
		luaSetStringField(L, -2, "fullname", fullname);
		luaSetUnsignedField(L, -2, "hosttarget", recordtype);
		luaSetUnsignedField(L, -2, "hosttarget", recordclass);
		luaSetUnsignedField(L, -2, "ttl", ttl);
		luaSetLStringField(L, -2, "data", data, datalen);
		endReplyCallback("query reply");
	}

	static int query(lua_State *L) 
	{
		luaMandatoryStringParam(L, 1, fullname);
		luaMandatoryUnsignedParam(L, 2, recordtype);
		luaMandatoryCallbackParam(L, 3, ctx);
		luaOptionalUnsignedParam(L, 4, _interface, 0);
		luaOptionalUnsignedParam(L, 5, flags, 0);
		luaOptionalUnsignedParam(L, 6, recordclass, kDNSServiceClass_IN);
		int error = DNSServiceQueryRecord(&ctx->client, flags, _interface, fullname, recordtype, recordclass, queryReply, ctx);
		returnContext("query request");
	}

	static int handle(lua_State *L) 
	{
		luaMandatoryUserDataParam(L, 1, context);
		luaOptionalUnsignedParam(L, 2, timeout, 5000);
		Context* ctx = (Context*)context;
		int dns_sd_fd  = DNSServiceRefSockFD(ctx->client);
		int nfds = dns_sd_fd + 1;
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(dns_sd_fd , &readfds);
		timeval tv;
		tv.tv_sec  = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;
		int result = select(nfds, &readfds, (fd_set*)NULL, (fd_set*)NULL, &tv);
		if (result > 0)
		{
			DNSServiceErrorType error = kDNSServiceErr_NoError;
			if(FD_ISSET(dns_sd_fd , &readfds)) 
				error = DNSServiceProcessResult(ctx->client);
			if (kDNSServiceErr_NoError != error) 
				return luaerror(L, "DNSServiceProcessResult() failed", errno);
		}
		else if(result < 0) //EINTR???
			return luaerror(L, "select() failed", errno);
		else
		{
			lua_pushboolean(L, false);
			return 1;
		}
		lua_pushboolean(L, true);
		return 1;
	}

	static int stop(lua_State *L) 
	{
		luaMandatoryUserDataParam(L, 1, context);
		delete (Context*)context;
		lua_pushboolean(L, true);
		return 1;
	}

	static void reg(lua_State *L)
	{
		lua_newtable(L); 
		luaSetCFunctionField(L, -2, "browse", conf0::browse);
		luaSetCFunctionField(L, -2, "resolve", conf0::resolve);
		luaSetCFunctionField(L, -2, "query", conf0::query);
		luaSetCFunctionField(L, -2, "handle", conf0::handle);
		luaSetCFunctionField(L, -2, "stop", conf0::stop);
		lua_setglobal(L, "conf0");
	}
};


void main()
{
	shared_ptr<lua_State> L(luaL_newstate(), lua_close);
	luaL_openlibs(&*L);
	conf0::reg(&*L);
    if(luaL_loadfile(&*L, "test.lua") || lua_pcall(&*L, 0, 0, 0))    
		printf("ERROR: %s\n", lua_tostring(&*L, -1));
}
