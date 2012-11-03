#!/usr/bin/ruby

require 'mkmf'

channel9ext = "channel9ext"
collector = with_config("collector") || 'semispace'
collector_class = collector.capitalize

$CFLAGS = "-Wall -Werror -frtti -Wno-unused-but-set-variable " # -Winline"
$CFLAGS << "-DCOLLECTOR=" + collector + " -DCOLLECTOR_CLASS=" + collector_class
$LDFLAGS = "-lstdc++"

traces = with_config("trace") || ""
traces = traces.split(',')

subscription = 0
subscription |= (2**30-1) if traces.include?('all')
subscription |= 1 if traces.include?('general')
subscription |= 2 if traces.include?('vm')
subscription |= 4 if traces.include?('gc')
subscription |= 8 if traces.include?('alloc')
$CFLAGS << " -DTRACE_SUB=" + subscription.to_s

levels = {
  'spam'  => 0,
  'debug' => 1,
  'info'  => 2,
  'warn'  => 3,
  'error' => 4,
  'crit'  => 5,
}
debug_level = with_config("trace-level") || 'warn'
$CFLAGS << " -DTRACE_LEVEL=" + (levels[debug_level] || debug_level).to_s


if enable_config("debug")
  $CFLAGS << " -DDEBUG -O0 -g"
else
  $CFLAGS << " -DNDEBUG -O4 -g" # --param inline-unit-growth=100000 --param large-function-growth=100000 --param max-inline-insns-single=100000"
end

dir_config(channel9ext)
create_makefile(channel9ext)

