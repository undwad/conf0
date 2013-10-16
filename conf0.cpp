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

	luaM_func_begin(connect)
		luaM_return(lightuserdata, (void*)-1)
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

	conf0_callback_begin(resolve_callback, uint32_t interface_, DNSServiceErrorType error, const char* fullname, const char* hosttarget, uint16_t opaqueport, uint16_t textlen, const unsigned char* text)
		luaM_setfield(-1, integer, interface_, interface_)
		luaM_setfield(-1, string, fullname, fullname)
		luaM_setfield(-1, string, hosttarget, hosttarget)
		luaM_setfield(-1, integer, opaqueport, opaqueport)
		luaM_setfield(-1, integer, textlen, textlen)
		luaM_setfield(-1, lstring, text, (const char*)text, textlen)
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
		luaM_opt_param(integer, textlen, 0)
		luaM_opt_param(string, text, nullptr)
		luaM_reqd_param(function, callback)
		luaM_return_userdata(context_t, context_t, context, L, callback)
		conf0_call_dns_service(DNSServiceRegister, &context->ref, (DNSServiceFlags)flags, interface_, name, type, domain, host, port, textlen, text, register_callback, context)
	luaM_func_end

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

#	undef IN
#	define conf0_reg_class(NAME) luaM_setfield(-1, integer, NAME, kDNSServiceClass_##NAME)
	void conf0_reg_classes(lua_State *L) // query | query callback
	{
		conf0_reg_class(IN)
	}

#	define conf0_reg_type(NAME) luaM_setfield(-1, integer, NAME, kDNSServiceType_##NAME)
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

#elif defined(LINUX)

#   include <avahi-client/client.h>
#   include <avahi-client/lookup.h>

#   include <avahi-common/simple-watch.h>
#   include <avahi-common/malloc.h>
#   include <avahi-common/error.h>

	const char* backend = "avahi";

#	define conf0_callback_begin(CALLBACK, ...) \
	void CALLBACK(__VA_ARGS__, void* userdata) \
	{ \
		base_context_t* context = (base_context_t*)userdata; \
		lua_State *L = context->L; \
		lua_rawgeti(L, LUA_REGISTRYINDEX, context->callback); \
		lua_newtable(L);

#	define conf0_callback_end(CALLBACK) \
		if(lua_pcall(L, 1, 0, 0)) \
		{ \
			fprintf(stderr, #CALLBACK " error %s\n", lua_tostring(L, -1)); \
			lua_pop(L, 1); \
		} \
	}

# define conf0_call_dns_service(RESULT, FUNC, ...) \
	context->RESULT = FUNC(__VA_ARGS__); \
	if(!context->RESULT) \
	{ \
		delete context; \
		return luaL_error(L, avahi_strerror(avahi_client_errno(client_context->client))); \
	}

	/* CONNECT */

	struct context_t
	{
		lua_State* L;
		int callback;
		virtual void init(lua_State* L_, int callback_)
		{
            L = L_;
            callback = callback_;
        }
		virtual ~context_t()
		{
			if(LUA_NOREF != callback)
				luaL_unref(L, LUA_REGISTRYINDEX, callback);
		}
	};

	struct client_context_t : context_t
	{
        AvahiSimplePoll* poll;
        AvahiClient* client;
		virtual void init(lua_State* L_, int callback_)
		{
            context_t::init(L_, callback_);
            poll = nullptr;
            client = nullptr;
		}
		virtual ~client_context_t()
		{
			if(client)
				avahi_client_free(client);
			if(poll)
				avahi_simple_poll_free(poll);
		}
	};

	struct base_context_t : context_t
	{
		client_context_t* client_context;
		virtual void init(lua_State* L_, int callback_, client_context_t* client_context_)
		{
            context_t::init(L_, callback_);
            client_context = client_context_;
        }
		virtual ~base_context_t() { }
	};

	luaM__gc(client_context_t)

	luaM_func_begin(iterate)
		luaM_reqd_param(userdata, ref)
		luaM_opt_param(integer, timeout, 1000)
		base_context_t* context = (base_context_t*)ref;
	    int res = avahi_simple_poll_iterate(context->client_context->poll, timeout);
        if(res < 0)
			return luaL_error(L, avahi_strerror(avahi_client_errno(context->client_context->client)));
		else if(res > 0)
		{
			luaM_return(boolean, false)
		}
		else
		{
			luaM_return(boolean, true)
		}
	luaM_func_end

	conf0_callback_begin(client_callback, AvahiClient* client, AvahiClientState state)
		luaM_setfield(-1, integer, state, state)
	conf0_callback_end(client_callback)

	luaM_func_begin(connect)
        luaM_opt_param(integer, flags, 0)
		luaM_reqd_param(function, callback)
		luaM_return_userdata(client_context_t, init, context, L, callback)
        if(context->poll = avahi_simple_poll_new())
        {
            int error;
            if(context->client = avahi_client_new(avahi_simple_poll_get(context->poll), flags, client_callback, context, &error))
				return result;
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

	/* BROWSE */

	struct browse_context_t : base_context_t
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
		luaM_reqd_param(userdata, client)
		client_context_t* client_context = (client_context_t*)client;
		luaM_opt_param(integer, interface_, 0)
		luaM_opt_param(integer, protocol, AVAHI_PROTO_UNSPEC)
		luaM_reqd_param(string, type)
		luaM_opt_param(string, domain, nullptr)
		luaM_opt_param(integer, flags, 0)
		luaM_reqd_param(function, callback)
		luaM_return_userdata(browse_context_t, init, context, L, callback, client_context)
		conf0_call_dns_service(browser, avahi_service_browser_new, client_context->client, (AvahiIfIndex)interface_, (AvahiProtocol)protocol, type, domain, (AvahiLookupFlags)flags, browse_callback, context)
	luaM_func_end

	/* RESOLVE */

	struct resolve_context_t : base_context_t
	{
		AvahiServiceResolver* resolver;
		virtual ~resolve_context_t()
		{
			if(resolver)
				avahi_service_resolver_free(resolver);
		}
	};

	luaM__gc(resolve_context_t)

	conf0_callback_begin(resolve_callback, AvahiServiceResolver *resolver, AvahiIfIndex interface_, AvahiProtocol protocol, AvahiResolverEvent event_, const char *name, const char *type, const char *domain, const char *targethost, const AvahiAddress *a, uint16_t opaqueport, AvahiStringList *txt, AvahiLookupResultFlags flags)
		luaM_setfield(-1, integer, interface_, interface_)
		luaM_setfield(-1, integer, protocol, protocol)
		luaM_setfield(-1, integer, event_, event_)
		luaM_setfield(-1, string, name, name)
		luaM_setfield(-1, string, type, type)
		luaM_setfield(-1, string, domain, domain)
		luaM_setfield(-1, string, targethost, targethost)
		luaM_setfield(-1, integer, opaqueport, opaqueport)
		luaM_setfield(-1, integer, flags, flags)
	conf0_callback_end(resolve_callback)

	luaM_func_begin(resolve)
		luaM_reqd_param(userdata, client)
		client_context_t* client_context = (client_context_t*)client;
		luaM_opt_param(integer, interface_, 0)
		luaM_opt_param(integer, protocol, AVAHI_PROTO_UNSPEC)
		luaM_reqd_param(string, name)
		luaM_reqd_param(string, type)
		luaM_reqd_param(string, domain)
		luaM_opt_param(integer, aprotocol, AVAHI_PROTO_UNSPEC)
		luaM_opt_param(integer, flags, 0)
		luaM_reqd_param(function, callback)
		luaM_return_userdata(resolve_context_t, init, context, L, callback, client_context)
		conf0_call_dns_service(resolver, avahi_service_resolver_new, client_context->client, (AvahiIfIndex)interface_, (AvahiProtocol)protocol, name, type, domain, (AvahiProtocol)aprotocol, (AvahiLookupFlags)flags, resolve_callback, context)
	luaM_func_end

	/* QUERY */

	struct query_context_t : base_context_t
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
		luaM_reqd_param(userdata, client)
		client_context_t* client_context = (client_context_t*)client;
		luaM_opt_param(integer, interface_, 0)
		luaM_opt_param(integer, protocol, AVAHI_PROTO_UNSPEC)
		luaM_reqd_param(string, fullname)
		luaM_reqd_param(integer, type)
		luaM_reqd_param(integer, class_)
		luaM_opt_param(integer, flags, 0)
		luaM_reqd_param(function, callback)
		luaM_return_userdata(query_context_t, init, context, L, callback, client_context)
		conf0_call_dns_service(browser, avahi_record_browser_new, client_context->client, (AvahiIfIndex)interface_, (AvahiProtocol)protocol, fullname, type, class_, (AvahiLookupFlags)flags, query_callback, context)
	luaM_func_end

	/* REGISTER */

	luaM_func_begin(register_)
		luaM_opt_param(integer, flags, 0)
		luaM_opt_param(integer, interface_, 0)
		luaM_opt_param(string, name, nullptr)
		luaM_reqd_param(string, type)
		luaM_opt_param(string, domain, nullptr)
		luaM_opt_param(string, host, nullptr)
		luaM_opt_param(integer, port, 0)
		luaM_opt_param(integer, textlen, 0)
		luaM_opt_param(string, text, nullptr)
		luaM_reqd_param(function, callback)
		//luaM_return_userdata(context_t, init, context, L, callback)
		//conf0_call_dns_service(DNSServiceRegister, &context->ref, (DNSServiceFlags)flags, interface_, name, type, domain, host, port, textlen, text, register_callback, context)
	luaM_func_end

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

#	define conf0_reg_class(NAME) luaM_setfield(-1, integer, NAME, kDNSServiceClass_##NAME)
	void conf0_reg_classes(lua_State *L) // query | query callback
	{
	}

#	define conf0_reg_type(NAME) luaM_setfield(-1, integer, NAME, kDNSServiceType_##NAME)
	void conf0_reg_types(lua_State *L) // query | query callback
	{
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

#   define conf0_reg_state(NAME) luaM_setfield(-1, integer, NAME, AVAHI_CLIENT_##NAME)
	void conf0_reg_states(lua_State *L)
	{
        conf0_reg_state(S_REGISTERING)
        conf0_reg_state(S_RUNNING)
        conf0_reg_state(S_COLLISION)
        conf0_reg_state(FAILURE)
        conf0_reg_state(CONNECTING)
	}

#elif defined(OSX)
#	error incompatible platform
#else
#	error incompatible platform
#endif

static const struct luaL_Reg lib[] =
{
	{"savestack", luaM_save_stack},
	{"connect", connect},
	{"browse", browse},
	{"resolve", resolve},
	{"query", query},
	{"register_", register_},
	{"iterate", iterate},
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
