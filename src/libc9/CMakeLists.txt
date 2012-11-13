add_library (libc9 SHARED
    callable_context.cpp
    context.cpp
    environment.cpp
    gc_markcompact.cpp
    gc_semispace.cpp
    instruction.cpp
    istream.cpp
    loader.cpp
    memory_pool.cpp
    message.cpp
    primitive.cpp
    string.cpp
    trace.cpp
    tuple.cpp
    value.cpp
    variable_frame.cpp
)
target_link_libraries(libc9 dl jsoncpp)
set_target_properties(libc9 PROPERTIES OUTPUT_NAME c9)
