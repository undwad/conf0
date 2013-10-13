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

static char error_text[1024] = {0};
int error_code = 0;

const char* conf0_error_text() { return error_text; }
int conf0_error_code() { return error_code; }

void set_error(const char* text, int code)
{
	strcpy(error_text, text);
	error_code = code;
}

#if defined(WIN32)
#	include "dns_sd.h"
#	pragma comment(lib, "dnssd.lib")
#	pragma comment(lib, "ws2_32.lib")

#	define CALLBACKDEF(CALLBACK, ...) \
	void DNSSD_API CALLBACK(DNSServiceRef ref, DNSServiceFlags flags, uint32_t interface_, DNSServiceErrorType error, __VA_ARGS__, void *context)

#	define CALLBACKBODY(FUNC, CALLBACK, ...) \
	{ \
		context_t* context_ = (context_t*)context; \
		if(kDNSServiceErr_NoError != error) set_error(#FUNC##"() failed", error); \
		((CALLBACK)context_->callback)(ref, flags, interface_, kDNSServiceErr_NoError != error, __VA_ARGS__, context_->userdata); \
		delete context_; \
	}

#	define FUNCDEF(FUNC, CALLBACK, ...) \
	void* conf0_enumdomain_alloc(void* common_context, unsigned int flags, unsigned int interface_, __VA_ARGS__, conf0_enumdomain_callback callback, void* userdata)

#	define FUNCBODY(FUNC, ...) \
	{ \
		DNSServiceRef ref = nullptr; \
		context_t* context = new context_t(callback, userdata); \
		DNSServiceErrorType error = FUNC(&ref, (DNSServiceFlags)flags, interface_, __VA_ARGS__, context); \
		if(kDNSServiceErr_NoError == error) return ref; \
		set_error(#FUNC##"() failed", error); \
		delete context; \
		return nullptr; \
	}

#define FREEPROC(FUNC) \
	void FUNC(void* context) { DNSServiceRefDeallocate((DNSServiceRef)context); }

	struct context_t
	{
		void* callback;
		void* userdata;
		context_t(void* callback_, void* userdata_) : callback(callback_), userdata(userdata_) { }
	};

	/* COMMON */

	void* conf0_common_alloc() { printf("alloc\n"); return (void*)-1; }
	void conf0_common_free(void* common_context) { printf("free %d\n", common_context); }

	int conf0_iterate(void* context, int timeout)
	{
		DNSServiceRef ref = (DNSServiceRef)context;
		int dns_sd_fd  = DNSServiceRefSockFD(ref);
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
				error = DNSServiceProcessResult(ref);
			if (kDNSServiceErr_NoError == error) 
				return 1;
			else
			{
				set_error("DNSServiceProcessResult() failed", error); 
				return -1;
			}
		}
		else if(result < 0) //EINTR???
		{
			set_error("select() failed", errno); 
			return -1;
		}
		else
			return 0;
	}

	/* DOMAIN */

	CALLBACKDEF(enumdomain_callback, const char *domain)
		CALLBACKBODY(DNSServiceEnumerateDomains, conf0_enumdomain_callback, domain)
	FUNCDEF(conf0_enumdomain_alloc, conf0_enumdomain_callback)
		FUNCBODY(DNSServiceEnumerateDomains, enumdomain_callback)
	FREEPROC(conf0_enumdomain_free)

	/* BROWSER */

	CALLBACKDEF(browser_callback, const char* name, const char* type, const char* domain)
		CALLBACKBODY(DNSServiceBrowse, conf0_browser_callback, name, type, domain)
	FUNCDEF(conf0_browser_alloc, conf0_browser_callback, const char* type, const char* domain)
		FUNCBODY(DNSServiceBrowse, type, domain, browser_callback)
	FREEPROC(conf0_browser_free)

	/* RESOLVER */

	CALLBACKDEF(resolver_callback, const char* fullname, const char* hosttarget, uint16_t port, uint16_t textlen, const unsigned char* text)
		CALLBACKBODY(DNSServiceResolve, conf0_resolver_callback, fullname, hosttarget, port, textlen, text)
	FUNCDEF(conf0_resolver_alloc, conf0_resolver_callback, const char* name, const char* type, const char* domain)
		FUNCBODY(DNSServiceResolve, name, type, domain, resolver_callback)
	FREEPROC(conf0_resolver_free)

	/* QUERY */

	CALLBACKDEF(query_callback, const char* fullname, uint16_t type, uint16_t class_, uint16_t datalen, const void* data, uint32_t ttl)
		CALLBACKBODY(DNSServiceQueryRecord, conf0_query_callback, fullname, type, class_, datalen, data, ttl)
	FUNCDEF(conf0_query_alloc, conf0_query_callback, const char* fullname, unsigned short type, unsigned short class_)
		FUNCBODY(DNSServiceQueryRecord, fullname, type, class_, query_callback)
	FREEPROC(conf0_query_free)
#elif defined(LINUX)
#elif defined(OSX)
#else
#	error incompatible platform
#endif
