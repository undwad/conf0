/*
** conf0.cpp by undwad
** crossplatform (bonjour (windows/osx), avahi(linux)) zeroconf(mdns) module for lua 5.2 
** https://github.com/undwad/conf0 mailto:undwad@mail.ru
** see copyright notice in conf0.h
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

#include "conf0.h"

static char error[1024] = {0};
const char* conf0_get_last_error() { return error; }


#if defined(WIN32)
#	include "dns_sd.h"
#	pragma comment(lib, "dnssd.lib")
#	pragma comment(lib, "ws2_32.lib")

#	define CALLBACKBODY(FUNC, CALLBACK, ...) \
	{ \
		context_t* context_ = (context_t*)context; \
		if(kDNSServiceErr_NoError != error) sprintf(::error, #FUNC##"() failed with error %d", error); \
		((CALLBACK)context_->callback)(ref, flags, interface_, kDNSServiceErr_NoError != error, __VA_ARGS__, context_->userdata); \
		delete context_; \
	}


#	define FUNCBODY(FUNC, ...) \
	{ \
		DNSServiceRef ref = nullptr; \
		context_t* context = new context_t(callback, userdata); \
		DNSServiceErrorType error = FUNC(&ref, (DNSServiceFlags)flags, interface_, __VA_ARGS__, context); \
		if(kDNSServiceErr_NoError == error) return ref; \
		sprintf(::error, #FUNC##"() failed with error %d", error); \
		delete context; \
		return nullptr; \
	}

	struct context_t
	{
		void* callback;
		void* userdata;
		context_t(void* callback_, void* userdata_) : callback(callback_), userdata(userdata_) { }
	};

	/* COMMON */

	void* conf0_common_alloc() { return nullptr; }
	void conf0_common_free(void* common_context) { }

	/* DOMAIN */

	void DNSSD_API enumdomain_callback(DNSServiceRef ref, DNSServiceFlags flags, uint32_t interface_, DNSServiceErrorType error, const char *domain, void *context)
		CALLBACKBODY(DNSServiceEnumerateDomains, conf0_enumdomain_callback, domain)
	void* conf0_enumdomain_alloc(void* common_context, unsigned int flags, unsigned int interface_, conf0_enumdomain_callback callback, void* userdata)
		FUNCBODY(DNSServiceEnumerateDomains, enumdomain_callback)
	void conf0_enumdomain_free(void* enumdomain_context) { DNSServiceRefDeallocate((DNSServiceRef)enumdomain_context); }

	/* BROWSER */

	void DNSSD_API browser_callback(DNSServiceRef ref, DNSServiceFlags flags, uint32_t interface_, DNSServiceErrorType error, const char* name, const char* type, const char* domain, void* context)
		CALLBACKBODY(DNSServiceBrowse, conf0_browser_callback, name, type, domain)
	void* conf0_browser_alloc(void* common_context, unsigned int flags, unsigned int interface_, const char* type, const char* domain, conf0_browser_callback callback, void* userdata)
		FUNCBODY(DNSServiceBrowse, type, domain, browser_callback)
	void conf0_browser_free(void* browser_context) { DNSServiceRefDeallocate((DNSServiceRef)browser_context); }

	/* RESOLVER */

	void DNSSD_API resolver_callback(DNSServiceRef ref, DNSServiceFlags flags, uint32_t interface_, DNSServiceErrorType error, const char* fullname, const char* hosttarget, uint16_t port, uint16_t textlen, const unsigned char* text, void* context)
		CALLBACKBODY(DNSServiceResolve, conf0_resolver_callback, fullname, hosttarget, port, textlen, text)
	void* conf0_resolver_alloc(void* common_context, unsigned int flags, unsigned int interface_, const char* name, const char* type, const char* domain, conf0_resolver_callback callback, void* userdata)
		FUNCBODY(DNSServiceResolve, name, type, domain, resolver_callback)
	void conf0_resolver_free(void* resolver_context) { DNSServiceRefDeallocate((DNSServiceRef)resolver_context); }

	/* QUERY */

	void* conf0_query_alloc(void* common_context, unsigned int flags, unsigned int interface_, const char* fullname, unsigned short type, unsigned short class_, conf0_query_callback callback, void* userdata)	
	{
	}

	void conf0_query_free(void* query_context) { DNSServiceRefDeallocate((DNSServiceRef)query_context); }
#elif defined(LINUX)
#elif defined(OSX)
#else
#	error incompatible platform
#endif
