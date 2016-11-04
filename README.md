# conf0

**crossplatform (bonjour (windows/osx), avahi(linux)) zeroconf(dnssd) module for lua 5.2**

conf0 allow lua code to browse network for devices, query device parameters (for example ip:port) and register new services.

## WINDOWS

conf0 depends on apple bonjour sdk that can be downloaded from https://developer.apple.com/bonjour/

also conf0 depends on lua 5.2 that can be downloaded from http://luabinaries.sourceforge.net/

required header **dns_sd.h** and libraries **liblua52.a** and dnssd.lib are in the root folder.

also there is visual c++ project file **conf0.vcxproj**.

if you have incompatible version of visual studio then simply create new dynamic library project, 
add project source and header files, module definition file and libraries.
anyway **CMakeLists.txt** is provided so it is possible to use cmake.

## LINUX

conf0 depends on avahi (ubuntu 13 by default only misses **livavahi-ui-dev** that can be installed with apt-get).

also conf0 depends on *lua5.2* and *liblua5.2* that can be installed with apt-get.

anyway cmake will tell you what's missing.

unfortunately I don't how to configure cmake to produce libraries without lib prefix,
so after building *libconf0.so* module you need either rename it to *conf0.so* or create appropriate symbolic link,
so that lua could find it.

## COMMON

all functions of the module use fake named parameters calling mechanism, 
that is only one parameter (wich is lua table) is accepted, like this:
`conf0.browse{type = '_http._tcp', callback = function(t) end}`

conf0 module provides following functions:

	`connect` - connect to avahi service (on bonjour does nothing),
	
	`disconnect`  - disconnects from avahi service (on bonjour does nothing),
	
	`iterate`  - iterate connection,
	
	`browse` - browse network for services,
	
	`resolve` - resolve service name,
	
	`query` - query service record, 
	
	`register_` - register new service.
	
functions `connect`, `browse`, `resolve`, `query` and `register_` have `callback` parameter. 
each callback accepts one argument which is also lua table, that contains service callback parameters.

possible values of parameters `class_`, `error`, `event_`, `flags`, `protocol`, `state` and `type` can be found in 
`conf0.classes`, `conf0.errors`, `conf0.events`, `conf0.flags`, `conf0.protocols`, `conf0.states` and `conf0.types` enumerations.

unfortunately bonjour and avahi are not so similar so you should take note of their differences when using conf0,
because of this conf0 has `conf0.backend` parameter that can contain `bonjour` or `avahi` string.

for more detailed information you can look through:
- ~~**output/test.lua**~~
- **output/test_browse.lua** - browsing for services example
- **output/test_register.lua** - registering a sevice example
- **output/browse0conf.lua** - lua helper functions used in examples

2013.10.18 14.12.01 - 2016.11.04 15.32.39 undwad, samara, russia 