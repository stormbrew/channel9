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
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

include_directories(include)

add_library (libc9 SHARED
    src/libc9/callable_context.cpp
    src/libc9/context.cpp
    src/libc9/environment.cpp
    src/libc9/gc_markcompact.cpp
    src/libc9/gc_semispace.cpp
    src/libc9/instruction.cpp
    src/libc9/istream.cpp
    src/libc9/memory_pool.cpp
    src/libc9/message.cpp
    src/libc9/primitive.cpp
    src/libc9/string.cpp
    src/libc9/trace.cpp
    src/libc9/tuple.cpp
    src/libc9/value.cpp
    src/libc9/variable_frame.cpp
    # Note: channel9ext.[hc]pp left out deliberately. They're for the ruby ext.
)
set_target_properties(libc9 PROPERTIES OUTPUT_NAME c9)
set(LIBS ${LIBS} libc9)

add_library(testext SHARED
    src/testext/testext.cpp
)

include_directories(src/jsoncpp)
add_library(jsoncpp
    src/jsoncpp/json_reader.cpp
    src/jsoncpp/json_value.cpp
    src/jsoncpp/json_writer.cpp
)
set(LIBS ${LIBS} jsoncpp)

add_executable (c9
    src/c9/c9.cpp
)
target_link_libraries (c9 dl ${LIBS})

add_custom_command(
    OUTPUT ${LIBRARY_OUTPUT_PATH}/c9-env/testext/testext.so
    COMMAND ${CMAKE_COMMAND} -E copy ${LIBRARY_OUTPUT_PATH}/libtestext.so ${LIBRARY_OUTPUT_PATH}/c9-env/testext/testext.so
    DEPENDS testext
)
add_custom_target(testext-pkg-so ALL DEPENDS ${LIBRARY_OUTPUT_PATH}/c9-env/testext/testext.so)
add_custom_command(
    OUTPUT ${LIBRARY_OUTPUT_PATH}/c9-env/testext/testext.c9l
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/src/testext/testext.c9l ${LIBRARY_OUTPUT_PATH}/c9-env/testext/testext.c9l
    DEPENDS ${CMAKE_SOURCE_DIR}/src/testext/testext.c9l
)
add_custom_target(testext-pkg-c9l ALL DEPENDS ${LIBRARY_OUTPUT_PATH}/c9-env/testext/testext.c9l)
add_custom_command(
    OUTPUT ${LIBRARY_OUTPUT_PATH}/c9-env/testext/testext.c9b
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/src/testext/testext.c9b ${LIBRARY_OUTPUT_PATH}/c9-env/testext/testext.c9b
    DEPENDS ${CMAKE_SOURCE_DIR}/src/testext/testext.c9b
)
add_custom_target(testext-pkg-c9b ALL DEPENDS ${LIBRARY_OUTPUT_PATH}/c9-env/testext/testext.c9b)
