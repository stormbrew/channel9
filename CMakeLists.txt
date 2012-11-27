cmake_minimum_required (VERSION 2.6)

set (CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type: Debug or Release")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release)

project (channel9)

set (channel9_VERSION_MAJOR 1)
set (channel9_VERSION_MINOR 0)

find_package(Ruby REQUIRED)

add_definitions( -D__STDC_FORMAT_MACROS )

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 --param inline-unit-growth=100000 --param large-function-growth=100000 --param max-inline-insns-single=100000 -Wall -Werror -frtti -Wno-unused-but-set-variable -Wno-strict-aliasing")

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    add_definitions( -DDEBUG )
else()
    add_definitions( -DNDEBUG )
endif()

option (ENABLE_VALGRIND "Enable valgrind instrumentation." OFF)
if(ENABLE_VALGRIND)
    add_definitions( -DVALGRIND )
else(ENABLE_VALGRIND)
    add_definitions( -DNVALGRIND )
endif(ENABLE_VALGRIND)

set (TRACE_LEVEL_GENERAL "OFF" CACHE STRING "Hard minimum trace level for general trace info (SPAM, DEBUG, INFO, WARN, ERROR, CRIT, OFF)")
set_property(CACHE TRACE_LEVEL_GENERAL PROPERTY STRINGS SPAM DEBUG INFO WARN ERROR CRIT OFF)

set (TRACE_LEVEL_VM "OFF" CACHE STRING "Hard minimum trace level for general trace info (SPAM, DEBUG, INFO, WARN, ERROR, CRIT, OFF)")
set_property(CACHE TRACE_LEVEL_VM PROPERTY STRINGS SPAM DEBUG INFO WARN ERROR CRIT OFF)

set (TRACE_LEVEL_GC "OFF" CACHE STRING "Hard minimum trace level for general trace info (SPAM, DEBUG, INFO, WARN, ERROR, CRIT, OFF)")
set_property(CACHE TRACE_LEVEL_GC PROPERTY STRINGS SPAM DEBUG INFO WARN ERROR CRIT OFF)

set (TRACE_LEVEL_ALLOC "OFF" CACHE STRING "Hard minimum trace level for general trace info (SPAM, DEBUG, INFO, WARN, ERROR, CRIT, OFF)")
set_property(CACHE TRACE_LEVEL_ALLOC PROPERTY STRINGS SPAM DEBUG INFO WARN ERROR CRIT OFF)

add_definitions(
    -DHARD_TRACE_LEVEL_GENERAL=VAL_TRACE_${TRACE_LEVEL_GENERAL}
    -DHARD_TRACE_LEVEL_VM=VAL_TRACE_${TRACE_LEVEL_VM}
    -DHARD_TRACE_LEVEL_GC=VAL_TRACE_${TRACE_LEVEL_GC}
    -DHARD_TRACE_LEVEL_ALLOC=VAL_TRACE_${TRACE_LEVEL_ALLOC}
)

if (DEFINED ENV{DEFAULT_COLLECTOR})
    set (DEFAULT_COLLECTOR $ENV{DEFAULT_COLLECTOR})
    message("Setting default collector to $ENV{DEFAULT_COLLECTOR}")
elseif()
    set (DEFAULT_COLLECTOR "Markcompact")
    message("Defaulting to mark compact collector.")
endif()

set (COLLECTOR ${DEFAULT_COLLECTOR} CACHE STRING "Which collector to use for long-lived objects. (Markcompact or Semispace)")
set_property(CACHE COLLECTOR PROPERTY STRINGS Markcompact Semispace)
add_definitions( -DCOLLECTOR_CLASS=${COLLECTOR} )

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

include_directories(include)

add_subdirectory(src/libc9)
add_subdirectory(src/jsoncpp)
add_subdirectory(src/c9)

function(add_c9script SRC DST COMMON_PATH TARGET_PREFIX) # extra args are script names.
    foreach(FILE ${ARGN})
        add_custom_command(
            OUTPUT ${DST}/${COMMON_PATH}/${FILE}.c9b
            COMMAND ${RUBY_EXECUTABLE} -rubygems -I${CMAKE_SOURCE_DIR}/ruby/lib ${CMAKE_SOURCE_DIR}/bin/c9c ${SRC}/${COMMON_PATH}/${FILE} ${DST}/${COMMON_PATH}/${FILE}.c9b
            DEPENDS ${SRC}/${COMMON_PATH}/${FILE}
        )
        add_custom_target(${TARGET_PREFIX}-${FILE} ALL DEPENDS ${DST}/${COMMON_PATH}/${FILE}.c9b)
    endforeach()
endfunction()

function(add_c9_alias NAME)
    add_custom_command(
            OUTPUT ${PROJECT_BINARY_DIR}/bin/c9.${NAME}
            COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_BINARY_DIR}/bin/c9 ${PROJECT_BINARY_DIR}/bin/c9.${NAME}
            DEPENDS ${PROJECT_BINARY_DIR}/bin/c9
    )
    add_custom_target(c9.${NAME} ALL DEPENDS ${PROJECT_BINARY_DIR}/bin/c9.${NAME})
endfunction()

file(GLOB environments "environments/*")
foreach(environment ${environments})
    add_subdirectory(${environment})
endforeach()
