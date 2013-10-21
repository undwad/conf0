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

#if defined(WIN32)
#	include "dns_sd.h"
#	pragma comment(lib, "dnssd.lib")
#	pragma comment(lib, "ws2_32.lib")

	const char* backend = "bonjour";

#	define conf0_callback_begin(CALLBACK, ...) \
	void DNSSD_API CALLBACK(DNSServiceRef ref, DNSServiceFlags flags, __VA_ARGS__, void* userdata) \
	{ \
		context_t* context = (context_t*)userdata; \
		lua_State *L = context->L; \
		lua_rawgeti(L, LUA_REGISTRYINDEX, context->callback); \
		lua_newtable(L); \
		luaM_setfield(-1, lightuserdata, ref, userdata) \
		luaM_setfield(-1, integer, flags, flags)

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

	luaM_func_begin(connect)
		luaM_reqd_param(function, callback)
		lua_rawgeti(L, LUA_REGISTRYINDEX, callback);
		lua_newtable(L); 
		int error = lua_pcall(L, 1, 0, 0);
		luaL_unref(L, LUA_REGISTRYINDEX, callback);
		if(error)
			return lua_error(L);
	luaM_func_end

	luaM_func_begin(disconnect)
	luaM_func_end

	luaM_func_begin(iterate)
		luaM_reqd_param(userdata, ref)
		luaM_opt_param(integer, timeout, 1000)
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
		{
			luaM_return(boolean, false)
		}
	luaM_func_end

	/* BROWSE */

	conf0_callback_begin(browse_callback, uint32_t interface_, DNSServiceErrorType error, const char* name, const char* type, const char* domain)
		luaM_setfield(-1, integer, interface_, interface_)
		luaM_setfield(-1, string, name, name)
		luaM_setfield(-1, string, type, type)
		luaM_setfield(-1, string, domain, domain)
	conf0_callback_end(browse_callback)

	luaM_func_begin(browse)
		luaM_opt_param(integer, flags, 0)
		luaM_opt_param(integer, interface_, 0)
		luaM_reqd_param(string, type)
		luaM_opt_param(string, domain, nullptr)
		luaM_reqd_param(function, callback)
		luaM_return_userdata(context_t, context_t, context, L, callback)
		conf0_call_dns_service(DNSServiceBrowse, &context->ref, (DNSServiceFlags)flags, interface_, type, domain, browse_callback, context)
	luaM_func_end

	/* RESOLVE */

	conf0_callback_begin(resolve_callback, uint32_t interface_, DNSServiceErrorType error, const char* fullname, const char* host, uint16_t opaqueport, uint16_t textlen, const unsigned char* text)
		luaM_setfield(-1, integer, interface_, interface_)
		luaM_setfield(-1, string, fullname, fullname)
		luaM_setfield(-1, string, host, host)
		luaM_setfield(-1, integer, opaqueport, opaqueport)
		lua_newtable(L);
		for(int i = 0, index = 0; i < textlen; i++)
		{
			int len = text[i];
			if(text[i] > 0)
			{
				lua_pushlstring(L, (char*)text + i + 1, len);
				lua_rawseti(L, -2, ++index);
				i += len;
			}
		}
		lua_setfield(L, -2, "texts");
	conf0_callback_end(resolve_callback)

	luaM_func_begin(resolve)
		luaM_opt_param(integer, flags, 0)
		luaM_opt_param(integer, interface_, 0)
		luaM_reqd_param(string, name)
		luaM_reqd_param(string, type)
		luaM_reqd_param(string, domain)
		luaM_reqd_param(function, callback)
		luaM_return_userdata(context_t, context_t, context, L, callback)
		conf0_call_dns_service(DNSServiceResolve, &context->ref, (DNSServiceFlags)flags, interface_, name, type, domain, resolve_callback, context)
	luaM_func_end

	/* QUERY */

	conf0_callback_begin(query_callback, uint32_t interface_, DNSServiceErrorType error, const char* fullname, uint16_t type, uint16_t class_, uint16_t datalen, const void* data, uint32_t ttl)
		luaM_setfield(-1, integer, interface_, interface_)
		luaM_setfield(-1, string, fullname, fullname)
		luaM_setfield(-1, integer, type, type)
		luaM_setfield(-1, integer, class_, class_)
		luaM_setfield(-1, integer, datalen, datalen)
		luaM_setfield(-1, lstring, data, (const char*)data, datalen)
	conf0_callback_end(query_callback)

	luaM_func_begin(query)
		luaM_opt_param(integer, flags, 0)
		luaM_opt_param(integer, interface_, 0)
		luaM_reqd_param(string, fullname)
		luaM_reqd_param(integer, type)
		luaM_reqd_param(integer, class_)
		luaM_reqd_param(function, callback)
		luaM_return_userdata(context_t, context_t, context, L, callback)
		conf0_call_dns_service(DNSServiceQueryRecord, &context->ref, (DNSServiceFlags)flags, interface_, fullname, type, class_, query_callback, context)
	luaM_func_end

	/* REGISTER */

	conf0_callback_begin(register_callback, DNSServiceErrorType error, const char* name, const char* type, const char* domain)
		luaM_setfield(-1, string, name, name)
		luaM_setfield(-1, string, type, type)
		luaM_setfield(-1, string, domain, domain)
	conf0_callback_end(register_callback)

	luaM_func_begin(register_)
		luaM_opt_param(integer, flags, 0)
		luaM_opt_param(integer, interface_, 0)
		luaM_opt_param(string, name, nullptr)
		luaM_reqd_param(string, type)
		luaM_opt_param(string, domain, nullptr)
		luaM_opt_param(string, host, nullptr)
		luaM_opt_param(integer, port, 0)
		luaM_opt_param(integer, textlen, 1)
		luaM_opt_param(string, text, "\0")
		luaM_reqd_param(function, callback)
		luaM_return_userdata(context_t, context_t, context, L, callback)
		conf0_call_dns_service(DNSServiceRegister, &context->ref, (DNSServiceFlags)flags, interface_, name, type, domain, host, port, textlen, text, register_callback, context)
	luaM_func_end

    /* ENUMS */

#	define conf0_reg_flag(NAME) luaM_setfield(-1, integer, NAME, kDNSServiceFlags##NAME)
	void conf0_reg_flags(lua_State *L)
	{
		conf0_reg_flag(MoreComing) // browse callback | resolve callback | query callback
		conf0_reg_flag(Add) // browse callback | query callback | register callback
		conf0_reg_flag(Default)
		conf0_reg_flag(NoAutoRename)
		conf0_reg_flag(Shared)
		conf0_reg_flag(Unique)
		conf0_reg_flag(BrowseDomains)
		conf0_reg_flag(RegistrationDomains)
		conf0_reg_flag(LongLivedQuery) // query
		conf0_reg_flag(AllowRemoteQuery)
		conf0_reg_flag(ForceMulticast) // resolve | query
		conf0_reg_flag(Force)
		conf0_reg_flag(ReturnIntermediates)
		conf0_reg_flag(NonBrowsable)
		conf0_reg_flag(ShareConnection)
		conf0_reg_flag(SuppressUnusable)
	}

#	define conf0_reg_error(NAME) luaM_setfield(-1, integer, NAME, kDNSServiceErr_##NAME)
	void conf0_reg_errors(lua_State *L)
	{
		conf0_reg_error(NoError)
		conf0_reg_error(Unknown)
		conf0_reg_error(NoSuchName)
		conf0_reg_error(NoMemory)
		conf0_reg_error(BadParam)
		conf0_reg_error(BadReference)
		conf0_reg_error(BadState)
		conf0_reg_error(BadFlags)
		conf0_reg_error(Unsupported)
		conf0_reg_error(NotInitialized)
		conf0_reg_error(AlreadyRegistered)
		conf0_reg_error(NameConflict)
		conf0_reg_error(Invalid)
		conf0_reg_error(Firewall)
		conf0_reg_error(Incompatible)
		conf0_reg_error(BadInterfaceIndex)
		conf0_reg_error(Refused)
		conf0_reg_error(NoSuchRecord)
		conf0_reg_error(NoAuth)
		conf0_reg_error(NoSuchKey)
		conf0_reg_error(NATTraversal)
		conf0_reg_error(DoubleNAT)
		conf0_reg_error(BadTime)
		conf0_reg_error(BadSig)
		conf0_reg_error(BadKey)
		conf0_reg_error(Transient)
		conf0_reg_error(ServiceNotRunning)
		conf0_reg_error(NATPortMappingUnsupported)
		conf0_reg_error(NATPortMappingDisabled)
		conf0_reg_error(NoRouter)
		conf0_reg_error(PollingMode)
		conf0_reg_error(Timeout)
	}

	void conf0_reg_protocols(lua_State *L) { }

	void conf0_reg_events(lua_State *L)	{ }

	void conf0_reg_states(lua_State *L) { }

	void conf0_reg_interfaces(lua_State *L) { luaM_setfield(-1, integer, UNSPEC, 0) }

#elif defined(LINUX)

#   include <unistd.h>

#   include <avahi-client/client.h>
#   include <avahi-client/lookup.h>
#   include <avahi-common/simple-watch.h>
#   include <avahi-common/malloc.h>
#   include <avahi-common/error.h>
#   include <avahi-client/publish.h>

	const char* backend = "avahi";

#	define conf0_callback_begin(CALLBACK, ...) \
	void CALLBACK(__VA_ARGS__, void* userdata) \
	{ \
		context_t* context = (context_t*)userdata; \
		lua_State *L = context->L; \
		lua_rawgeti(L, LUA_REGISTRYINDEX, context->callback); \
		lua_newtable(L); \
		luaM_setfield(-1, lightuserdata, ref, userdata)

#	define conf0_callback_end(CALLBACK) \
		if(lua_pcall(L, 1, 0, 0)) \
		{ \
			fprintf(stderr, #CALLBACK " error %s\n", lua_tostring(L, -1)); \
			lua_pop(L, 1); \
		} \
	}

#	define conf0_call_dns_service(RESULT, FUNC, ...) \
		context->RESULT = FUNC(__VA_ARGS__); \
		if(!context->RESULT) \
		{ \
			delete context; \
			return luaL_error(L, avahi_strerror(avahi_client_errno(context->client))); \
		}

#	define conf0_call_dns_service_proc(PROC, ...) \
		if(PROC(__VA_ARGS__) < 0) \
		{ \
			delete context; \
			return luaL_error(L, avahi_strerror(avahi_client_errno(context->client))); \
		}

	/* CONNECT */

	struct context_t
	{
		lua_State* L;
		int callback;
        AvahiSimplePoll* poll;
        AvahiClient* client;
		bool copy;
        context_t() : L(nullptr), callback(LUA_NOREF), poll(nullptr), client(nullptr), copy(false) {}
		virtual void init(lua_State* L_, int callback_, context_t* context)
		{
            L = L_;
            callback = callback_;
            if(context)
            {
                poll = context->poll;
                client = context->client;
                copy = true;
            }
        }
		virtual ~context_t()
		{
			if(LUA_NOREF != callback)
				luaL_unref(L, LUA_REGISTRYINDEX, callback);
			if(!copy)
			{
				if(client)
					avahi_client_free(client);
				if(poll)
					avahi_simple_poll_free(poll);
			}
		}
	};

    luaM__gc(context_t)

	conf0_callback_begin(client_callback, AvahiClient *client, AvahiClientState state)
		luaM_setfield(-1, integer, state, state)
		context->client = client; 
	conf0_callback_end(client_callback)

	luaM_func_begin(connect)
		luaM_opt_param(integer, flags, 0)
		luaM_reqd_param(function, callback)
		luaM_return_userdata(context_t, init, context, L, callback, nullptr)
        if(context->poll = avahi_simple_poll_new())
        {
            int error;
            if(context->client = avahi_client_new(avahi_simple_poll_get(context->poll), flags, client_callback, context, &error))
            {
            }
            else
            {
                delete context;
                return luaL_error(L, avahi_strerror(error));
            }
        }
        else
        {
            delete context;
            return luaL_error(L, "avahi_simple_poll_new() failed");
        }
	luaM_func_end

	luaM_func_begin(disconnect)
        luaM_reqd_param(userdata, ref)
		avahi_simple_poll_quit(((context_t*)ref)->poll);
	luaM_func_end

	luaM_func_begin(iterate)
		luaM_reqd_param(userdata, ref)
		luaM_opt_param(integer, timeout, 1000)
		context_t* context = (context_t*)ref;
	    int res = avahi_simple_poll_iterate(context->poll, timeout);
        if(res < 0)
			return luaL_error(L, avahi_strerror(avahi_client_errno(context->client)));
		else if(res > 0)
		{
			luaM_return(boolean, false)
		}
		else
		{
			luaM_return(boolean, true)
		}
	luaM_func_end

	/* BROWSE */

	struct browse_context_t : context_t
	{
		AvahiServiceBrowser* browser;
		virtual ~browse_context_t()
		{
			if(browser)
				avahi_service_browser_free(browser);
		}
	};

	luaM__gc(browse_context_t)

	conf0_callback_begin(browse_callback, AvahiServiceBrowser *browser, AvahiIfIndex interface_, AvahiProtocol protocol, AvahiBrowserEvent event_, const char *name, const char *type, const char *domain, AvahiLookupResultFlags flags)
		luaM_setfield(-1, integer, interface_, interface_)
		luaM_setfield(-1, integer, protocol, protocol)
		luaM_setfield(-1, integer, event_, event_)
		luaM_setfield(-1, string, name, name)
		luaM_setfield(-1, string, type, type)
		luaM_setfield(-1, string, domain, domain)
		luaM_setfield(-1, integer, flags, flags)
	conf0_callback_end(browse_callback)

	luaM_func_begin(browse)
        luaM_reqd_param(userdata, ref)
		luaM_opt_param(integer, interface_, AVAHI_IF_UNSPEC)
		luaM_opt_param(integer, protocol, AVAHI_PROTO_UNSPEC)
		luaM_reqd_param(string, type)
		luaM_opt_param(string, domain, nullptr)
		luaM_opt_param(integer, flags, 0)
		luaM_reqd_param(function, callback)
		luaM_return_userdata(browse_context_t, init, context, L, callback, (context_t*)ref)
		conf0_call_dns_service(browser, avahi_service_browser_new, context->client, (AvahiIfIndex)interface_, (AvahiProtocol)protocol, type, domain, (AvahiLookupFlags)flags, browse_callback, context)
	luaM_func_end

	/* RESOLVE */

	struct resolve_context_t : context_t
	{
		AvahiServiceResolver* resolver;
		virtual ~resolve_context_t()
		{
			if(resolver)
				avahi_service_resolver_free(resolver);
		}
	};

	luaM__gc(resolve_context_t)

	conf0_callback_begin(resolve_callback, AvahiServiceResolver *resolver, AvahiIfIndex interface_, AvahiProtocol protocol, AvahiResolverEvent event_, const char *name, const char *type, const char *domain, const char *host, const AvahiAddress *address, uint16_t port, AvahiStringList *txt, AvahiLookupResultFlags flags)
		luaM_setfield(-1, integer, interface_, interface_)
		luaM_setfield(-1, integer, protocol, protocol)
		luaM_setfield(-1, integer, event_, event_)
		luaM_setfield(-1, string, name, name)
		luaM_setfield(-1, string, type, type)
		luaM_setfield(-1, string, domain, domain)
		luaM_setfield(-1, string, host, host)
		luaM_setfield(-1, integer, port, port)
		luaM_setfield(-1, string, text, avahi_string_list_to_string(txt))
		if(address)
		{
			luaM_setfield(-1, integer, protocol, address->proto)
			char ip[AVAHI_ADDRESS_STR_MAX];
			avahi_address_snprint(ip, sizeof(ip), address);
			luaM_setfield(-1, string, ip, ip);
		}
		luaM_setfield(-1, integer, flags, flags)
	conf0_callback_end(resolve_callback)

	luaM_func_begin(resolve)
        luaM_reqd_param(userdata, ref)
		luaM_opt_param(integer, interface_, AVAHI_IF_UNSPEC)
		luaM_opt_param(integer, protocol, AVAHI_PROTO_UNSPEC)
		luaM_reqd_param(string, name)
		luaM_reqd_param(string, type)
		luaM_reqd_param(string, domain)
		luaM_opt_param(integer, aprotocol, AVAHI_PROTO_UNSPEC)
		luaM_opt_param(integer, flags, 0)
		luaM_reqd_param(function, callback)
		luaM_return_userdata(resolve_context_t, init, context, L, callback, (context_t*)ref)
		conf0_call_dns_service(resolver, avahi_service_resolver_new, context->client, (AvahiIfIndex)interface_, (AvahiProtocol)protocol, name, type, domain, (AvahiProtocol)aprotocol, (AvahiLookupFlags)flags, resolve_callback, context)
	luaM_func_end

	/* QUERY */

	struct query_context_t : context_t
	{
		AvahiRecordBrowser* browser;
		virtual ~query_context_t()
		{
			if(browser)
				avahi_record_browser_free(browser);
		}
	};

	luaM__gc(query_context_t)

	conf0_callback_begin(query_callback, AvahiRecordBrowser *resolver, AvahiIfIndex interface_, AvahiProtocol protocol, AvahiBrowserEvent event_, const char *fullname, uint16_t class_, uint16_t type, const void *data, size_t datalen, AvahiLookupResultFlags flags)
		luaM_setfield(-1, integer, interface_, interface_)
		luaM_setfield(-1, integer, protocol, protocol)
		luaM_setfield(-1, integer, event_, event_)
		luaM_setfield(-1, string, fullname, fullname)
		luaM_setfield(-1, integer, class_, class_)
		luaM_setfield(-1, integer, type, type)
		luaM_setfield(-1, string, data, (const char*)data)
		luaM_setfield(-1, integer, flags, flags)
	conf0_callback_end(query_callback)

	luaM_func_begin(query)
        luaM_reqd_param(userdata, ref)
		luaM_opt_param(integer, interface_, AVAHI_IF_UNSPEC)
		luaM_opt_param(integer, protocol, AVAHI_PROTO_UNSPEC)
		luaM_reqd_param(string, fullname)
		luaM_reqd_param(integer, type)
		luaM_reqd_param(integer, class_)
		luaM_opt_param(integer, flags, 0)
		luaM_reqd_param(function, callback)
		luaM_return_userdata(query_context_t, init, context, L, callback, (context_t*)ref)
		conf0_call_dns_service(browser, avahi_record_browser_new, context->client, (AvahiIfIndex)interface_, (AvahiProtocol)protocol, fullname, type, class_, (AvahiLookupFlags)flags, query_callback, context)
	luaM_func_end

	/* REGISTER */

	struct register_context_t : context_t
	{
		AvahiEntryGroup* group;
		virtual ~register_context_t()
		{
			if(group)
				avahi_entry_group_free(group);
		}
	};

	luaM__gc(register_context_t)

	conf0_callback_begin(group_callback, AvahiEntryGroup *group, AvahiEntryGroupState state)
		luaM_setfield(-1, integer, state, state)
	conf0_callback_end(group_callback)

	luaM_func_begin(register_)
        luaM_reqd_param(userdata, ref)
		luaM_opt_param(integer, flags, 0)
		luaM_opt_param(integer, interface_, AVAHI_IF_UNSPEC)
		luaM_opt_param(integer, protocol, AVAHI_PROTO_UNSPEC)
		luaM_opt_param(string, name, nullptr)
		luaM_reqd_param(string, type)
		luaM_opt_param(string, domain, nullptr)
		luaM_opt_param(string, host, nullptr)
		luaM_opt_param(integer, port, 0)
		luaM_opt_param(integer, textlen, 1)
		luaM_opt_param(string, text, "\0")
		luaM_reqd_param(function, callback)
		luaM_return_userdata(register_context_t, init, context, L, callback, (context_t*)ref)
		conf0_call_dns_service(group, avahi_entry_group_new, context->client, group_callback, context)
		AvahiStringList* list = avahi_string_list_add_arbitrary(nullptr, text, textlen);
		conf0_call_dns_service_proc(avahi_entry_group_add_service_strlst, context->group, interface_, protocol, flags, name, type, domain, host, port, list)
		avahi_string_list_free(list);
		conf0_call_dns_service_proc(avahi_entry_group_commit, context->group)
	luaM_func_end

    /* ENUMS */

#   define conf0_reg_flag(NAME) luaM_setfield(-1, integer, NAME, AVAHI_##NAME)
	void conf0_reg_flags(lua_State *L)
	{
		conf0_reg_flag(LOOKUP_USE_WIDE_AREA)
		conf0_reg_flag(LOOKUP_USE_MULTICAST)
		conf0_reg_flag(LOOKUP_NO_TXT)
		conf0_reg_flag(LOOKUP_NO_ADDRESS)
		conf0_reg_flag(LOOKUP_RESULT_CACHED)
		conf0_reg_flag(LOOKUP_RESULT_WIDE_AREA)
		conf0_reg_flag(LOOKUP_RESULT_MULTICAST)
		conf0_reg_flag(LOOKUP_RESULT_LOCAL)
		conf0_reg_flag(LOOKUP_RESULT_OUR_OWN)
		conf0_reg_flag(LOOKUP_RESULT_STATIC)
		conf0_reg_flag(CLIENT_IGNORE_USER_CONFIG)
		conf0_reg_flag(CLIENT_NO_FAIL)
	}

#	define conf0_reg_error(NAME) luaM_setfield(-1, integer, NAME, kDNSServiceErr_##NAME)
	void conf0_reg_errors(lua_State *L)
	{
	}

#   define conf0_reg_protocol(NAME) luaM_setfield(-1, integer, NAME, AVAHI_PROTO_##NAME)
	void conf0_reg_protocols(lua_State *L)
	{
        conf0_reg_protocol(INET)
        conf0_reg_protocol(INET6)
        conf0_reg_protocol(UNSPEC)
	}

#   define conf0_reg_event(NAME) luaM_setfield(-1, integer, NAME, AVAHI_##NAME)
	void conf0_reg_events(lua_State *L)
	{
        conf0_reg_event(BROWSER_NEW)
        conf0_reg_event(BROWSER_REMOVE)
        conf0_reg_event(BROWSER_CACHE_EXHAUSTED)
        conf0_reg_event(BROWSER_ALL_FOR_NOW)
        conf0_reg_event(BROWSER_FAILURE)
        conf0_reg_event(RESOLVER_FOUND)
        conf0_reg_event(RESOLVER_FAILURE)
	}

#   define conf0_reg_state(NAME) luaM_setfield(-1, integer, NAME, AVAHI_##NAME)
	void conf0_reg_states(lua_State *L)
	{
        conf0_reg_state(CLIENT_S_REGISTERING)
        conf0_reg_state(CLIENT_S_RUNNING)
        conf0_reg_state(CLIENT_S_COLLISION)
        conf0_reg_state(CLIENT_FAILURE)
        conf0_reg_state(CLIENT_CONNECTING)
        conf0_reg_state(ENTRY_GROUP_UNCOMMITED)
        conf0_reg_state(ENTRY_GROUP_REGISTERING)
        conf0_reg_state(ENTRY_GROUP_ESTABLISHED)
        conf0_reg_state(ENTRY_GROUP_COLLISION)
        conf0_reg_state(ENTRY_GROUP_FAILURE)
	}

	void conf0_reg_interfaces(lua_State *L) { luaM_setfield(-1, integer, UNSPEC, AVAHI_IF_UNSPEC) }

#	define kDNSServiceClass_IN        1

#	define kDNSServiceType_A         1      /* Host address. */
#	define kDNSServiceType_NS        2      /* Authoritative server. */
#	define kDNSServiceType_MD        3      /* Mail destination. */
#	define kDNSServiceType_MF        4      /* Mail forwarder. */
#	define kDNSServiceType_CNAME     5      /* Canonical name. */
#	define kDNSServiceType_SOA       6      /* Start of authority zone. */
#	define kDNSServiceType_MB        7      /* Mailbox domain name. */
#	define kDNSServiceType_MG        8      /* Mail group member. */
#	define kDNSServiceType_MR        9      /* Mail rename name. */
#	define kDNSServiceType_NULL      10     /* Null resource record. */
#	define kDNSServiceType_WKS       11     /* Well known service. */
#	define kDNSServiceType_PTR       12     /* Domain name pointer. */
#	define kDNSServiceType_HINFO     13     /* Host information. */
#	define kDNSServiceType_MINFO     14     /* Mailbox information. */
#	define kDNSServiceType_MX        15     /* Mail routing information. */
#	define kDNSServiceType_TXT       16     /* One or more text strings (NOT "zero or more..."). */
#	define kDNSServiceType_RP        17     /* Responsible person. */
#	define kDNSServiceType_AFSDB     18     /* AFS cell database. */
#	define kDNSServiceType_X25       19     /* X_25 calling address. */
#	define kDNSServiceType_ISDN      20     /* ISDN calling address. */
#	define kDNSServiceType_RT        21     /* Router. */
#	define kDNSServiceType_NSAP      22     /* NSAP address. */
#	define kDNSServiceType_NSAP_PTR  23     /* Reverse NSAP lookup (deprecated). */
#	define kDNSServiceType_SIG       24     /* Security signature. */
#	define kDNSServiceType_KEY       25     /* Security key. */
#	define kDNSServiceType_PX        26     /* X.400 mail mapping. */
#	define kDNSServiceType_GPOS      27     /* Geographical position (withdrawn). */
#	define kDNSServiceType_AAAA      28     /* IPv6 Address. */
#	define kDNSServiceType_LOC       29     /* Location Information. */
#	define kDNSServiceType_NXT       30     /* Next domain (security). */
#	define kDNSServiceType_EID       31     /* Endpoint identifier. */
#	define kDNSServiceType_NIMLOC    32     /* Nimrod Locator. */
#	define kDNSServiceType_SRV       33     /* Server Selection. */
#	define kDNSServiceType_ATMA      34     /* ATM Address */
#	define kDNSServiceType_NAPTR     35     /* Naming Authority PoinTeR */
#	define kDNSServiceType_KX        36     /* Key Exchange */
#	define kDNSServiceType_CERT      37     /* Certification record */
#	define kDNSServiceType_A6        38     /* IPv6 Address (deprecated) */
#	define kDNSServiceType_DNAME     39     /* Non-terminal DNAME (for IPv6) */
#	define kDNSServiceType_SINK      40     /* Kitchen sink (experimental) */
#	define kDNSServiceType_OPT       41     /* EDNS0 option (meta-RR) */
#	define kDNSServiceType_APL       42     /* Address Prefix List */
#	define kDNSServiceType_DS        43     /* Delegation Signer */
#	define kDNSServiceType_SSHFP     44     /* SSH Key Fingerprint */
#	define kDNSServiceType_IPSECKEY  45     /* IPSECKEY */
#	define kDNSServiceType_RRSIG     46     /* RRSIG */
#	define kDNSServiceType_NSEC      47     /* Denial of Existence */
#	define kDNSServiceType_DNSKEY    48     /* DNSKEY */
#	define kDNSServiceType_DHCID     49     /* DHCP Client Identifier */
#	define kDNSServiceType_NSEC3     50     /* Hashed Authenticated Denial of Existence */
#	define kDNSServiceType_NSEC3PARAM 51     /* Hashed Authenticated Denial of Existence */
#	define kDNSServiceType_HIP       55     /* Host Identity Protocol */
#	define kDNSServiceType_SPF       99     /* Sender Policy Framework for E-Mail */
#	define kDNSServiceType_UINFO     100    /* IANA-Reserved */
#	define kDNSServiceType_UID       101    /* IANA-Reserved */
#	define kDNSServiceType_GID       102    /* IANA-Reserved */
#	define kDNSServiceType_UNSPEC    103    /* IANA-Reserved */
#	define kDNSServiceType_TKEY      249    /* Transaction key */
#	define kDNSServiceType_TSIG      250    /* Transaction signature. */
#	define kDNSServiceType_IXFR      251    /* Incremental zone transfer. */
#	define kDNSServiceType_AXFR      252    /* Transfer zone of authority. */
#	define kDNSServiceType_MAILB     253    /* Transfer mailbox records. */
#	define kDNSServiceType_MAILA     254    /* Transfer mail agent records. */
#	define kDNSServiceType_ANY       255

#elif defined(OSX)
#	error incompatible platform
#else
#	error incompatible platform
#endif

#undef IN
#define conf0_reg_class(NAME) luaM_setfield(-1, integer, NAME, kDNSServiceClass_##NAME)
void conf0_reg_classes(lua_State *L) // query | query callback
{
	conf0_reg_class(IN)
}

#define conf0_reg_type(NAME) luaM_setfield(-1, integer, NAME, kDNSServiceType_##NAME)
void conf0_reg_types(lua_State *L) // query | query callback
{
	conf0_reg_type(A)
	conf0_reg_type(NS)
	conf0_reg_type(MD)
	conf0_reg_type(MF)
	conf0_reg_type(CNAME)
	conf0_reg_type(SOA)
	conf0_reg_type(MB)
	conf0_reg_type(MG)
	conf0_reg_type(MR)
	conf0_reg_type(WKS)
	conf0_reg_type(PTR)
	conf0_reg_type(HINFO)
	conf0_reg_type(MINFO)
	conf0_reg_type(MX)
	conf0_reg_type(TXT)
	conf0_reg_type(RP)
	conf0_reg_type(AFSDB)
	conf0_reg_type(X25)
	conf0_reg_type(ISDN)
	conf0_reg_type(RT)
	conf0_reg_type(NSAP)
	conf0_reg_type(NSAP_PTR)
	conf0_reg_type(SIG)
	conf0_reg_type(KEY)
	conf0_reg_type(PX)
	conf0_reg_type(GPOS)
	conf0_reg_type(AAAA)
	conf0_reg_type(LOC)
	conf0_reg_type(NXT)
	conf0_reg_type(EID)
	conf0_reg_type(NIMLOC)
	conf0_reg_type(SRV)
	conf0_reg_type(ATMA)
	conf0_reg_type(NAPTR)
	conf0_reg_type(KX)
	conf0_reg_type(CERT)
	conf0_reg_type(A6)
	conf0_reg_type(DNAME)
	conf0_reg_type(SINK)
	conf0_reg_type(OPT)
	conf0_reg_type(APL)
	conf0_reg_type(DS)
	conf0_reg_type(SSHFP)
	conf0_reg_type(IPSECKEY)
	conf0_reg_type(RRSIG)
	conf0_reg_type(NSEC)
	conf0_reg_type(DNSKEY)
	conf0_reg_type(DHCID)
	conf0_reg_type(NSEC3)
	conf0_reg_type(NSEC3PARAM)
	conf0_reg_type(HIP)
	conf0_reg_type(SPF)
	conf0_reg_type(UINFO)
	conf0_reg_type(UID)
	conf0_reg_type(GID)
	conf0_reg_type(UNSPEC)
	conf0_reg_type(TKEY)
	conf0_reg_type(TSIG)
	conf0_reg_type(IXFR)
	conf0_reg_type(AXFR)
	conf0_reg_type(MAILB)
	conf0_reg_type(MAILA)
	conf0_reg_type(ANY)
}

static const struct luaL_Reg lib[] =
{
	//{"savestack", luaM_save_stack},
	{"connect", connect},
	{"browse", browse},
	{"resolve", resolve},
	{"query", query},
	{"register_", register_},
	{"iterate", iterate},
	{"disconnect", disconnect},
    {nullptr, nullptr},
};

#define conf0_reg_enum(NAME) \
	lua_newtable(L); \
	conf0_reg_##NAME(L); \
	lua_setfield(L, -2, #NAME);

extern "C"
{
	LUAMOD_API int luaopen_conf0(lua_State *L)
	{
		luaL_newlib (L, lib);
		luaM_setfield(-1, string, backend, backend)
		conf0_reg_enum(flags)
		conf0_reg_enum(classes)
		conf0_reg_enum(types)
		conf0_reg_enum(errors)
		conf0_reg_enum(protocols)
		conf0_reg_enum(events)
		conf0_reg_enum(states)
		conf0_reg_enum(interfaces)
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
