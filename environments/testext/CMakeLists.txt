set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib/c9-env/testext)
add_library(testext SHARED
    testext.cpp
)
target_link_libraries(testext libc9 -Wl,--no-undefined)
add_custom_command(
    OUTPUT ${LIBRARY_OUTPUT_PATH}/testext.c9l
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/testext.c9l ${LIBRARY_OUTPUT_PATH}/testext.c9l
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/testext.c9l
)
add_custom_target(testext-pkg-c9l ALL DEPENDS ${LIBRARY_OUTPUT_PATH}/testext.c9l)

add_c9script(${CMAKE_CURRENT_SOURCE_DIR} ${LIBRARY_OUTPUT_PATH} . pkg-testext testext.c9s)

add_c9_alias(testext)
