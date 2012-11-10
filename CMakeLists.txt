cmake_minimum_required (VERSION 2.6)
project (channel9)

set (channel9_VERSION_MAJOR 1)
set (channel9_VERSION_MINOR 0)

if(DEFINED DEBUG)
    set (CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-g -O0")
    add_definitions( -DDEBUG -DTRACE_SUB=0xffffffff -DTRACE_LEVEL=0 )
else(DEFINED DEBUG)
    set (CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-g -O4 --param inline-unit-growth=100000 --param large-function-growth=100000 --param max-inline-insns-single=100000 -Wall -Werror -frtti -Wno-unused-but-set-variable -Wno-strict-aliasing")
    add_definitions( -DNDEBUG -DTRACE_SUB=0 -DTRACE_LEVEL=6 )
endif(DEFINED DEBUG)
if(DEFINED VALGRIND)
    add_definitions( -DVALGRIND )
else(DEFINED VALGRIND)
    add_definitions( -DNVALGRIND )
endif(DEFINED VALGRIND)
add_definitions( -DCOLLECTOR_CLASS=Markcompact -D__STDC_FORMAT_MACROS )

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

include_directories(include)

add_subdirectory(src/libc9)
add_subdirectory(src/jsoncpp)
add_subdirectory(src/c9)

file(GLOB environments "environments/*")
foreach(environment ${environments})
    add_subdirectory(${environment})
endforeach()
