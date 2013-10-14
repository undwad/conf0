/*
** conf0.cpp by undwad
** crossplatform (bonjour (windows/osx), avahi(linux)) zeroconf(mdns) module for lua 5.2 
** https://github.com/undwad/conf0 mailto:undwad@mail.ru
** see copyright notice in conf0.h
*/

#include "sysdefs.h"

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
		if(kDNSServiceErr_NoError != error) set_error(#FUNC##"() failed", error); \
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
		set_error(#FUNC##"() failed", error); \
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
#elif defined(OSX)
#else
#	error incompatible platform
#endif
