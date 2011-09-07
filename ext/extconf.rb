#!/usr/bin/ruby

require 'mkmf'

channel9ext = "channel9ext"
collector = with_config("collector") || 'semispace'
collector_class = collector.capitalize

$CFLAGS = "-Wall -Werror -frtti " # -Winline"
$CFLAGS << "-DCOLLECTOR=" + collector + " -DCOLLECTOR_CLASS=" + collector_class
$LDFLAGS = "-lstdc++"

if enable_config("trace")
  $CFLAGS << " -DTRACE"
end

if enable_config("tracegc")
  $CFLAGS << " -DTRACEGC"
end

if enable_config("debug")
  $CFLAGS << " -DDEBUG -O0 -g"
else
  $CFLAGS << " -DNDEBUG -O4 -g" # --param inline-unit-growth=100000 --param large-function-growth=100000 --param max-inline-insns-single=100000"
end

dir_config(channel9ext)
create_makefile(channel9ext)

