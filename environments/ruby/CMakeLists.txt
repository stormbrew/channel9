set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib/c9-env/rb)

include_directories(${RUBY_INCLUDE_DIRS})
add_definitions( -DC9RB_RUBY_LIB=\"${CMAKE_CURRENT_SOURCE_DIR}/lib\" -DC9_RUBY_LIB=\"${CMAKE_SOURCE_DIR}/ruby/lib\" )
add_library(rb SHARED
    src/rb.cpp
    src/rb_glob.cpp
    src/rb_loader.cpp
    src/rb_regexp.cpp
)
target_link_libraries(rb libc9 ${RUBY_LIBRARY})

add_custom_command(
    OUTPUT ${LIBRARY_OUTPUT_PATH}/rb.c9l
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/rb.c9l ${LIBRARY_OUTPUT_PATH}/rb.c9l
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/rb.c9l
)
add_custom_target(rb-pkg-c9l ALL DEPENDS ${LIBRARY_OUTPUT_PATH}/rb.c9l)

add_c9script(${CMAKE_CURRENT_SOURCE_DIR}/environment ${LIBRARY_OUTPUT_PATH} kernel/alpha rb-pkg-alpha
	basic_hash.c9s
	object.c9s
	class.c9s
	module.c9s
	finish.c9s
)

function(add_compiled_ruby SRC DST COMMON_PATH TARGET_PREFIX) # extra args are script names.
	foreach(FILE ${ARGN})
		add_custom_command(
			OUTPUT ${DST}/${COMMON_PATH}/${FILE}.c9b
			COMMAND ${RUBY_EXECUTABLE} -rubygems -I${CMAKE_SOURCE_DIR}/ruby/lib -I${CMAKE_CURRENT_SOURCE_DIR}/lib ${CMAKE_CURRENT_SOURCE_DIR}/bin/c9c.rb ${SRC}/${COMMON_PATH}/${FILE} ${DST}/${COMMON_PATH}/${FILE}.c9b
			DEPENDS ${SRC}/${COMMON_PATH}/${FILE}
		)
		add_custom_target(${TARGET_PREFIX}-${FILE} ALL DEPENDS ${DST}/${COMMON_PATH}/${FILE}.c9b)
	endforeach()
endfunction()

add_compiled_ruby(${CMAKE_CURRENT_SOURCE_DIR}/environment ${LIBRARY_OUTPUT_PATH} kernel/beta rb-pkg-beta
	singletons.rb
	module.rb
	string.rb
	symbol.rb
	kernel.rb
	enumerable.rb
	static_tuple.rb
	tuple.rb
	array.rb
	proc.rb
	exceptions.rb
	class.rb
)

add_compiled_ruby(${CMAKE_CURRENT_SOURCE_DIR}/environment ${LIBRARY_OUTPUT_PATH} kernel/delta rb-pkg-delta
	boot.rb
	channel9.rb
	class.rb
	comparable.rb
	dir.rb
	file.rb
	fixnum.rb
	float.rb
	hash.rb
	io.rb
	kernel.rb
	message.rb
	module.rb
	object.rb
	range.rb
	regexp.rb
	signal.rb
	string.rb
	table.rb
	thread.rb
	time.rb
)

add_compiled_ruby(${CMAKE_CURRENT_SOURCE_DIR}/environment ${LIBRARY_OUTPUT_PATH} site-lib site-lib rbconfig.rb)

add_custom_command(
    OUTPUT ${LIBRARY_OUTPUT_PATH}/lib
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/environment/lib ${LIBRARY_OUTPUT_PATH}/lib
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/environment/lib
)
add_custom_target(rb-pkg-stdlib ALL DEPENDS ${LIBRARY_OUTPUT_PATH}/lib)

add_c9_alias(rb)
