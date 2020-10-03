cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

if ("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
	message(WARNING "It is not recommended to build the project in the same directory with the sources.\nYou can move to your build dir and run cmake from it (don't forget delete cmake cache files)." )
endif ()

project(CBQueue)
set(CMAKE_C_STANDARD 99)

set(BASE_SOURCES cbqcontainer.c cbqcapacity.c cbqversion.c cbqueue.c cbqcallbacks.c) 
set(DEBUG_SOURCES cbqdebug.c cbqtest.c main.c)

add_library(CBQueue STATIC ${BASE_SOURCES})


if (CMAKE_BUILD_TYPE MATCHES DEBUG)
	add_executable(CBQueueDebug ${DEBUG_SOURCES})
	target_link_libraries(CBQueueDebug CBQueue)
endif ()
