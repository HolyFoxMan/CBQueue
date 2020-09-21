
cmake_minimum_required(VERSION 2.8)

project(CBQueue)

set(CMAKE_C_STANDARD 98)

set(BASE_OBJECTS cbqcontainer.c cbqcapacity.c cbqversion.c cbqueue.c) 
add_subdirectory(Release)
add_library(CBQueue ${BASE_OBJECTS})
target_link_libraries(CBQueue LINK_PUBLIC Release)

set(DEBUG_OBJECTS cbqdebug.c cbqtest.c main.c)
add_subdirectory(Debug)
add_executable(CBQueueDebug ${BASE_OBJECTS} ${DEBUG_OBJECTS})
target_link_libraries(CBQueueDebug Debug)
