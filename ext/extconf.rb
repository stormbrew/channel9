require 'mkmf'

channel9ext = "channel9ext"

$CFLAGS = "-Wall -Werror -Winline " + 
	"-O0 -g"
	#"-O3 --param inline-unit-growth=10000 --param large-function-growth=10000 --param max-inline-insns-single=10000"

dir_config(channel9ext)
create_makefile(channel9ext)
