find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBFFI REQUIRED libffi)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LIBFFI_CFLAGS} -I${CMAKE_SOURCE_DIR}/src/pegtl/include")

add_library (libc9 SHARED
    callable_context.cpp
    context.cpp
    environment.cpp
    ffi.cpp
    gc.cpp
    gc_markcompact.cpp
    gc_nursery.cpp
    gc_semispace.cpp
    instruction.cpp
    istream.cpp
    loader.cpp
    message.cpp
    primitive.cpp
    script.cpp
    string.cpp
    trace.cpp
    tuple.cpp
    value.cpp
    variable_frame.cpp
)
target_link_libraries(libc9 dl jsoncpp ${LIBFFI_LDFLAGS})
set_target_properties(libc9 PROPERTIES OUTPUT_NAME c9)
