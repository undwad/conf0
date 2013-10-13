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

