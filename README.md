conf0
=====
crossplatform (bonjour (windows/osx), avahi(linux)) zeroconf(dnssd) module for lua 5.2

conf0 allow lua code to browse network for devices, query device parameters (for example ip:port) and register new services.

on windows:
conf0 depends on apple bonjour sdk that can be downloaded from https://developer.apple.com/bonjour/
also conf0 depends on lua 5.2 that can be downloaded from http://luabinaries.sourceforge.net/
required header dns_sd.h and libraries liblua52.a and dnssd.lib are in the root folder.
also there is visual c++ project file conf0.vcxproj.
if you have incompatible version of visual studio then simply create new dynamic library project,
add project source and header files, module definition file and libraries.
anyway CMakeLists.txt is provided so it is possible to use cmake.

on linux:
conf0 depends on avahi (ubuntu 13 by default only misses livavahi-ui-dev that can be installed with apt-get).
also conf0 depends on lua5.2 and liblua5.2 that can be installed with apt-get.
anyway cmake will tell you what's missing.
unfortunately I don't how to configure cmake to produce libraries without lib prefix,
so after building libconf0.so module you need either rename it to conf0.so or create appropriate symbolic link,
so that lua could find it.

common:
all functions of the module use fake named parameters calling mechanism, 
that is only one parameter (wich is lua table) is accepted
conf0 module provides following functions

folder output contains test script test.lua, you can look through it for more detailed information.



2013.10.11 08.38.58 undwad, samara, russia