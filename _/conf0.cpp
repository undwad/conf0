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

	struct userdata_t
	{
		DNSServiceRef ref;
		void* callback;
		void* userdata;
		userdata_t(void* callback_, void* userdata_) : ref(nullptr), callback(callback_), userdata(userdata_) { }
	};

#	define BEGINCALLBACK(CALLBACK, ...) \
	void DNSSD_API CALLBACK(DNSServiceRef ref, DNSServiceFlags flags, uint32_t interface_, DNSServiceErrorType error, __VA_ARGS__, void* userdata) \
	{

#	define ENDCALLBACK(FUNC, CALLBACK, ...) \
		userdata_t* userdata_ = (userdata_t*)userdata; \
		if(kDNSServiceErr_NoError != error) set_error(#FUNC "() failed", error); \
		((CALLBACK)userdata_->callback)(ref, flags, interface_, kDNSServiceErr_NoError != error, __VA_ARGS__, userdata_->userdata); \
	}

#	define BEGINFUNC(FUNC, CALLBACK, ...) \
	void* FUNC(void* common_context, unsigned int flags, unsigned int interface_, __VA_ARGS__, CALLBACK callback, void* userdata) \
	{

#	define ENDFUNC(FUNC, ...) \
		DNSServiceRef ref = nullptr; \
		userdata_t* userdata_ = new userdata_t(callback, userdata); \
		DNSServiceErrorType error = FUNC(&userdata_->ref, (DNSServiceFlags)flags, interface_, __VA_ARGS__, userdata_); \
		if(kDNSServiceErr_NoError == error) return userdata_; \
		set_error(#FUNC "() failed", error); \
		delete userdata_; \
		return nullptr; \
	}

#define FREEPROC(FUNC) \
	void FUNC(void* userdata) \
	{  \
		userdata_t* userdata_ = (userdata_t*)userdata; \
		DNSServiceRefDeallocate(userdata_->ref); \
		delete userdata_; \
	}

	/* COMMON */

	void* conf0_common_alloc() { return (void*)-1; }
	void conf0_common_free(void* common_context) { }

	int conf0_iterate(void* userdata, int timeout)
	{
		userdata_t* userdata_ = (userdata_t*)userdata;
		int dns_sd_fd  = DNSServiceRefSockFD(userdata_->ref);
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
				error = DNSServiceProcessResult(userdata_->ref);
			if (kDNSServiceErr_NoError == error)
				return 0;
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
			return 1;
	}

	/* DOMAIN */

	extern const int browser_browse = kDNSServiceFlagsBrowseDomains;

	BEGINCALLBACK(enumdomain_callback, const char *domain)
	ENDCALLBACK(DNSServiceEnumerateDomains, conf0_enumdomain_callback, domain)

	BEGINFUNC(conf0_enumdomain_alloc, conf0_enumdomain_callback)
	ENDFUNC(DNSServiceEnumerateDomains, enumdomain_callback)

	FREEPROC(conf0_enumdomain_free)

	/* BROWSER */

	BEGINCALLBACK(browser_callback, const char* name, const char* type, const char* domain)
	ENDCALLBACK(DNSServiceBrowse, conf0_browser_callback, name, type, domain)

	BEGINFUNC(conf0_browser_alloc, conf0_browser_callback, const char* type, const char* domain)
	ENDFUNC(DNSServiceBrowse, type, domain, browser_callback)

	FREEPROC(conf0_browser_free)

	/* RESOLVER */

	BEGINCALLBACK(resolver_callback, const char* fullname, const char* hosttarget, uint16_t opaqueport, uint16_t textlen, const unsigned char* text)
	ENDCALLBACK(DNSServiceResolve, conf0_resolver_callback, fullname, hosttarget, opaqueport, textlen, text)

	BEGINFUNC(conf0_resolver_alloc, conf0_resolver_callback, const char* name, const char* type, const char* domain)
	ENDFUNC(DNSServiceResolve, name, type, domain, resolver_callback)

	FREEPROC(conf0_resolver_free)

	/* QUERY */

	extern const int record_class = kDNSServiceClass_IN;

	BEGINCALLBACK(query_callback, const char* fullname, uint16_t type, uint16_t class_, uint16_t datalen, const void* data, uint32_t ttl)
	ENDCALLBACK(DNSServiceQueryRecord, conf0_query_callback, fullname, type, class_, datalen, data, ttl)

	BEGINFUNC(conf0_query_alloc, conf0_query_callback, const char* fullname, unsigned short type, unsigned short class_)
	ENDFUNC(DNSServiceQueryRecord, fullname, type, class_, query_callback)

	FREEPROC(conf0_query_free)

	/* REGISTER */

	void DNSSD_API register_callback(DNSServiceRef ref, DNSServiceFlags flags, DNSServiceErrorType error, const char* name, const char* type, const char* domain, void* userdata)
	{
		uint32_t interface_ = 0;
	ENDCALLBACK(DNSServiceRegister, conf0_register_callback, name, type, domain)

	BEGINFUNC(conf0_register_alloc, conf0_register_callback, const char* name, const char* type, const char* domain, const char* host, unsigned short port, unsigned short textlen, const void* text)
	ENDFUNC(DNSServiceRegister, name, type, domain, host, port, textlen, text, register_callback)

	FREEPROC(conf0_register_free)

#elif defined(LINUX)

#   include <avahi-client/client.h>
#   include <avahi-client/lookup.h>

#   include <avahi-common/simple-watch.h>
#   include <avahi-common/malloc.h>
#   include <avahi-common/error.h>

#	define BEGINCALLBACK(CALLBACK, ...) \
    void CALLBACK(AvahiDomainBrowser *b, AvahiIfIndex interface, AvahiProtocol protocol, AvahiBrowserEvent event, __VA_ARGS__, AvahiLookupResultFlags flags, void *userdata) \
	{

#	define ENDCALLBACK(TYPE, ERROR, ...) \
        TYPE* context = (TYPE*)userdata; \
        if(ERROR == event) \
        { \
            int error = avahi_client_errno(context->client); \
            set_error(avahi_strerror(error), error); \
        } \
        else \
            context->callback(context, flags, interface, ERROR == event, __VA_ARGS__, context->userdata); \
    }

#	define BEGINFUNC(FUNC, CALLBACK, ...) \
	void* FUNC(void* common_context, unsigned int flags, __VA_ARGS__, CALLBACK callback, void* userdata) \
	{

#	define ENDFUNC(TYPE, FUNC, CALLBACK, ...) \
        common_t* common = (common_t*)common_context; \
        TYPE* context = new TYPE; \
        context->poll = common->poll; \
        context->client = common->client; \
        context->callback = callback; \
        context->userdata = userdata; \
        if(context->handle = FUNC(common->client, interface_, AVAHI_PROTO_UNSPEC, __VA_ARGS__, 0, CALLBACK, context)) \
            return context; \
        int error = avahi_client_errno(common->client); \
        set_error(avahi_strerror(error), error); \
        delete context; \
        return nullptr; \
	}


#   define FREEPROC(FUNC, TYPE, PROC) \
	void FUNC(void* context) \
	{  \
		TYPE* context_ = (TYPE*)context; \
		PROC(context_->handle); \
		delete context_; \
	}

	/* COMMON */

	struct common_t
	{
        AvahiSimplePoll* poll;
        AvahiClient* client;
	};

    static void client_callback(AvahiClient* client, AvahiClientState state, AVAHI_GCC_UNUSED void* userdata) { }

	void* conf0_common_alloc()
	{
        common_t* common = new common_t;
        if(common)
        {
            if(common->poll = avahi_simple_poll_new())
            {
                int error;
                if(common->client = avahi_client_new(avahi_simple_poll_get(common->poll), 0, client_callback, nullptr, &error))
                    return common;
                else set_error(avahi_strerror(error), error);
                avahi_simple_poll_free(common->poll);
            }
            else set_error("avahi_simple_poll_new() failed", 0);
            delete common;
        }
        return nullptr;
	}

	void conf0_common_free(void* common_context)
	{
        common_t* common = (common_t*)common_context;
        avahi_client_free(common->client);
        avahi_simple_poll_free(common->poll);
        delete common;
	}

	int conf0_iterate(void* userdata, int timeout)
	{
        common_t* common = (common_t*)userdata;
        int result = avahi_simple_poll_iterate(common->poll, timeout);
        if(result < 0)
        {
            int error = avahi_client_errno(common->client);
            set_error(avahi_strerror(error), error);
        }
        return result;
	}

	/* DOMAIN */
	extern const int browser_browse = AVAHI_DOMAIN_BROWSER_BROWSE;

    struct enumdomain_t : common_t
    {
        AvahiDomainBrowser* handle;
        conf0_enumdomain_callback callback;
        void* userdata;
    };

    BEGINCALLBACK(enumdomain_callback, const char *domain)
    ENDCALLBACK(enumdomain_t, AVAHI_BROWSER_FAILURE, domain)

    BEGINFUNC(conf0_enumdomain_alloc, conf0_enumdomain_callback, unsigned int interface_)
    ENDFUNC(enumdomain_t, avahi_domain_browser_new, enumdomain_callback, nullptr, flags)

    FREEPROC(conf0_enumdomain_free, enumdomain_t, avahi_domain_browser_free)

	/* BROWSER */

    struct browser_t : common_t
    {
        AvahiServiceBrowser* handle;
        conf0_browser_callback callback;
        void* userdata;
    };

    BEGINCALLBACK(browser_callback, const char *name, const char *type, const char *domain)
    ENDCALLBACK(browser_t, AVAHI_BROWSER_FAILURE, name, type, domain)

    BEGINFUNC(conf0_browser_alloc, conf0_browser_callback, unsigned int interface_, const char *type, const char *domain)
    ENDFUNC(browser_t, avahi_service_browser_new, browser_callback, type, domain)

    FREEPROC(conf0_browser_free, browser_t, avahi_service_browser_free)

	/* RESOLVER */

    struct resolver_t : common_t
    {
        AvahiServiceResolver* handle;
        conf0_resolver_callback callback;
        void* userdata;
    };

    BEGINCALLBACK(resolver_callback, const char *name, const char *type, const char *domain, const char *host_name, const AvahiAddress *address, uint16_t port, AvahiStringList *txt)
    ENDCALLBACK(resolver_t, AVAHI_RESOLVER_FAILURE, name, host_name, port, 0, nullptr)

    BEGINFUNC(conf0_resolver_alloc, conf0_resolver_callback, unsigned int interface_, const char *name, const char *type, const char *domain)
    ENDFUNC(resolver_t, avahi_service_resolver_new, resolver_callback, name, type, domain, AVAHI_PROTO_UNSPEC)

    FREEPROC(conf0_resolver_free, resolver_t, avahi_service_resolver_free)

	/* QUERY */
	extern const int record_class = 1;

	void* conf0_query_alloc(void* common_context, unsigned int flags, unsigned int interface_, const char* fullname, unsigned short type, unsigned short class_, conf0_query_callback callback, void* userdata) { return nullptr; }
    void conf0_query_free(void* query_context) {}

	/* REGISTER */

	void* conf0_register_alloc(void* common_context, unsigned int flags, unsigned int interface_, const char* name, const char* type, const char* domain, const char* host, unsigned short port, unsigned short textlen, const void* text, conf0_register_callback callback, void* userdata) { return nullptr; }
    void conf0_register_free(void* register_context) {}

#elif defined(OSX)
#else
#	error incompatible platform
#endif