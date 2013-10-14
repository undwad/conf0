cmake_minimum_required(VERSION 2.6)

set (PROJECT conf0)
project (${PROJECT})

SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-fpermissive")

set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")

FIND_PACKAGE(Lua)
if (LUA_FOUND)
  message("Lua was found!")
  message(${LUA_INCLUDE_DIR})
  message(${LUA_LIBRARY})
endif (LUA_FOUND)

file (GLOB FILES
	"*.h"
	"*.cpp"
)

include_directories(${LUA_INCLUDE_DIR})

add_library(${PROJECT} SHARED ${FILES})

target_link_libraries (${PROJECT} ${LUA_LIBRARY})


