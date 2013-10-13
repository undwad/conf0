/*
** conf0.h by undwad
** crossplatform (bonjour (windows/osx), avahi(linux)) zeroconf(mdns) module for lua 5.2 
** https://github.com/undwad/conf0 mailto:undwad@mail.ru
** see copyright notice at the end of this file
*/

#ifndef _CONF0_H__
#define _CONF0_H__

#if !defined(nullptr)
#	define nullptr NULL
#endif

const char* conf0_get_last_error();

/* COMMON */

void* conf0_common_alloc();
void conf0_common_free(void* common_context);

/* DOMAIN */

typedef void (*conf0_enumdomain_callback)
(
	void* enumdomain_context, 
	unsigned int flags, 
	unsigned int interface_, 
	bool error, 
	const char* domain, 
	void* userdata
);
void* conf0_enumdomain_alloc
(
	void* common_context, 
	unsigned int flags, 
	unsigned int interface_, 
	conf0_enumdomain_callback callback, 
	void* userdata
);
void conf0_enumdomain_free(void* enumdomain_context);

/* BROWSER */

typedef void (*conf0_browser_callback)
(
	void* browser_context, 
	unsigned int flags, 
	unsigned int interface_, 
	bool error, 
	const char* name, 
	const char* type, 
	const char* domain, 
	void* userdata
);
void* conf0_browser_alloc
(
	void* common_context, 
	unsigned int flags, 
	unsigned int interface_, 
	const char* type, 
	const char* domain, 
	conf0_browser_callback callback, 
	void* userdata
);
void conf0_browser_free(void* browser_context);

/* RESOLVER */

typedef void (*conf0_resolver_callback)
(
	void* resolver_context, 
	unsigned int flags, 
	unsigned int interface_, 
	bool error, 
	const char* fullname, 
	const char* hosttarget, 
	unsigned short port, 
	unsigned short textlen, 
	const unsigned char* text, 
	void* userdata
);
void* conf0_resolver_alloc
(
	void* common_context, 
	unsigned int flags, 
	unsigned int interface_, 
	const char* name,
	const char* type,
	const char* domain, 
	conf0_resolver_callback callback, 
	void* userdata
);
void conf0_resolver_free(void* resolver_context);

/* QUERY */

typedef void (*conf0_query_callback)
(
	void* query_context, 
	unsigned int flags, 
	unsigned int interface_, 
	bool error, 
	const char* fullname, 
	unsigned short type, 
	unsigned short class_,
	unsigned short port, 
	unsigned short datalen, 
	const void* data, 
	unsigned int ttl,
	void* userdata
);
void* conf0_query_alloc
(
	void* common_context, 
	unsigned int flags, 
	unsigned int interface_, 
	const char* fullname,
	unsigned short type,
	unsigned short class_, 
	conf0_query_callback callback, 
	void* userdata
);
void conf0_query_free(void* query_context);

/* REGISTER */

#endif // _CONF0_H__

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

