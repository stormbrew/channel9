cmake_minimum_required (VERSION 2.6)
project (channel9)

set (channel9_VERSION_MAJOR 1)
set (channel9_VERSION_MINOR 0)

if(DEFINED DEBUG)
    set (CMAKE_CXX_FLAGS "-g -O0")
    add_definitions( -DDEBUG -DTRACE_SUB=0xffffffff -DTRACE_LEVEL=0 )
else(DEFINED DEBUG)
    set (CMAKE_CXX_FLAGS "-g -O4 --param inline-unit-growth=100000 --param large-function-growth=100000 --param max-inline-insns-single=100000 -Wall -Werror -frtti -Wno-unused-but-set-variable -Wno-strict-aliasing")
    add_definitions( -DNDEBUG -DTRACE_SUB=0 -DTRACE_LEVEL=6 )
endif(DEFINED DEBUG)
add_definitions( -DCOLLECTOR_CLASS=Markcompact -D__STDC_FORMAT_MACROS )

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

include_directories(ext)

add_library (c9
    ext/bittwiddle.hpp
    ext/callable_context.hpp
    ext/callable_context.cpp
    ext/channel9.hpp
    ext/context.hpp
    ext/context.cpp
    ext/environment.hpp
    ext/environment.cpp
    ext/forwardtable.hpp
    ext/gc_markcompact.hpp
    ext/gc_markcompact.cpp
    ext/gc_nursery.hpp
    ext/gc_semispace.hpp
    ext/gc_semispace.cpp
    ext/instruction.hpp
    ext/instruction.cpp
    ext/istream.hpp
    ext/istream.cpp
    ext/memcheck.h
    ext/memory_pool.hpp
    ext/memory_pool.cpp
    ext/message.hpp
    ext/message.cpp
    ext/primitive.hpp
    ext/primitive.cpp
    ext/string.hpp
    ext/string.cpp
    ext/trace.hpp
    ext/trace.cpp
    ext/tuple.hpp
    ext/tuple.cpp
    ext/valgrind.h
    ext/value.hpp
    ext/value.cpp
    ext/variable_frame.hpp
    ext/variable_frame.cpp
)
set(LIBS ${LIBS} -Wl,-whole-archive c9 -Wl,-no-whole-archive)

include_directories(src/jsoncpp)
add_library(jsoncpp
    src/jsoncpp/json_batchallocator.h
    src/jsoncpp/json_internalarray.inl
    src/jsoncpp/json_internalmap.inl
    src/jsoncpp/json_reader.cpp
    src/jsoncpp/json_tool.h
    src/jsoncpp/json_value.cpp
    src/jsoncpp/json_valueiterator.inl
    src/jsoncpp/json_writer.cpp
)
set(LIBS ${LIBS} jsoncpp)

add_executable (c9-bin
    src/c9/c9.cpp
)
set_target_properties(c9-bin PROPERTIES OUTPUT_NAME c9)
target_link_libraries (c9-bin ${LIBS})
