#!/usr/bin/env ruby
if $0 =~ %r{bin/c9.rb$}
  $LOAD_PATH.unshift "../channel9/lib"
  $LOAD_PATH.unshift "../channel9.rb/lib"
  require 'rubygems'
end

require 'channel9'
require 'channel9/loader/ruby'

debug = ARGV.include?("-d")
ARGV.delete("-d")
debug = ARGV.include?("-dd") ? :detail : debug
ARGV.delete("-dd")
print = ARGV.include?("-p")
ARGV.delete("-p")
if (ARGV.include?("-v"))
  puts("Channel9.rb 0.0.0.0.1")
  ARGV.delete("-v")
end

loader = Channel9::Loader::Ruby.new(debug)
stream = loader.compile(ARGV.shift)
loader.set_argv(ARGV)
if (print)
  puts stream.to_json
else
  context = Channel9::Context.new(loader.env, stream)
  global_self = loader.env.special_channel[:global_self]
  context.channel_send(loader.env, global_self, Channel9::CleanExitChannel)
end