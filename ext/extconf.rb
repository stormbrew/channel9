require 'mkmf'

channel9ext = "channel9ext"

$CFLAGS = "-Wall -Werror -Winline"

if enable_config("trace")
  $CFLAGS << " -DTRACE"
end

if enable_config("debug") 
  $CFLAGS << " -DDEBUG -O0 -g"
else
  $CFLAGS << " -DNO_DEBUG -O3 --param inline-unit-growth=10000 --param large-function-growth=10000 --param max-inline-insns-single=10000"
end

dir_config(channel9ext)
create_makefile(channel9ext)
