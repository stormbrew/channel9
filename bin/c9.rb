#!/usr/bin/env ruby
if $0 == "bin/c9.rb"
  $LOAD_PATH.unshift "../channel9/lib"
  $LOAD_PATH.unshift "lib"
end

require 'channel9'
require 'channel9/loader/ruby'

debug = ARGV.include?("-d")
ARGV.delete("-d")
print = ARGV.include?("-p")
ARGV.delete("-p")

loader = Channel9::Loader::Ruby.new(debug)
stream = loader.compile(ARGV[0])
if (print)
  puts stream.to_json
else
  context = Channel9::Context.new(loader.env, stream)
  global_self = loader.env.special_channel[:global_self]
  context.channel_send(global_self, Channel9::CleanExitChannel)
end