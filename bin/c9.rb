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
if (print)
  stream = loader.compile(ARGV.shift)
  puts stream.to_json
else
  exe = Channel9::Primitive::String.new($0)
  filename = Channel9::Primitive::String.new(ARGV.shift)
  loader.setup_environment(exe, ARGV)
  global_self = loader.env.special_channel[:global_self]
  loader.env.save_context do
    global_self.channel_send(loader.env, Channel9::Primitive::Message.new(:raw_load, [], [filename]), Channel9::CleanExitChannel)
  end
end