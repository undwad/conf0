/*
** conf0.cpp by undwad
** crossplatform (bonjour (windows/osx), avahi(linux)) zeroconf(mdns) module for lua 5.2
** https://github.com/undwad/conf0 mailto:undwad@mail.ru
** see copyright notice in conf0.h
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

