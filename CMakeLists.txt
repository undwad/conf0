cmake_minimum_required(VERSION 2.6)

set (PROJECT conf0)
project (${PROJECT})

SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-fpermissive")

set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")

FIND_PACKAGE(Lua 5.2)
if (LUA_FOUND)
  message("Lua was found!")
  message(${LUA_INCLUDE_DIR})
  message(${LUA_LIBRARY})
endif (LUA_FOUND)

FIND_PACKAGE(AVAHI)
if (AVAHI_FOUND)
  message("AVAHI was found!")
  message(${AVAHI_INCLUDE_DIRS})
  message(${AVAHI_LIBRARIES})
endif (AVAHI_FOUND)

file (GLOB FILES
	"*.h"
	"*.c"
	"*.cpp"
)

include_directories(${LUA_INCLUDE_DIR} ${AVAHI_INCLUDE_DIRS})

add_library(${PROJECT} SHARED ${FILES})

target_link_libraries (${PROJECT} ${LUA_LIBRARY} ${AVAHI_LIBRARIES})


