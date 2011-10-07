#!/usr/bin/env ruby
if $0 =~ %r{bin/c9.rb$}
  $LOAD_PATH.unshift "../channel9/lib"
  $LOAD_PATH.unshift "../channel9/ext"
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
compile = ARGV.include?("-c")
ARGV.delete("-c")
timing = ARGV.include?("-t")
ARGV.delete("-t")
# trace all, start off tracing before the script itself is run.
Channel9::Environment.trace = ARGV.include?("-TT")
ARGV.delete("-TT")
trace = ARGV.include?("-T")
ARGV.delete("-T")
if (ARGV.include?("-v"))
  puts("Channel9.rb 0.0.0.0.1")
  ARGV.delete("-v")
end
stime = Time.now
loader = Channel9::Loader::Ruby.new(debug)
if (print)
  stream = Channel9::Loader::Ruby.compile(ARGV.shift)
  puts stream.to_json
elsif (compile)
  infile = ARGV.shift
  stream = Channel9::Loader::Ruby.compile(infile)
  outfile = infile.gsub(%r{/([^/]+?)(\.rb)?$}) do |m|
    "/#{$1}.c9b"
  end
  File.open(outfile, "w") do |f|
    f.write stream.to_json
  end
else
  exe = $0
  filename = ARGV.shift
  loader.setup_environment(exe, ARGV)
  Channel9::Environment.trace = true if trace
  global_self = loader.env.special_channel(:global_self)
  loader_time = Time.now
  begin
    loader.env.save_context do
      global_self.channel_send(loader.env, Channel9::Primitive::Message.new(:load, [], [filename]), Channel9::CleanExitChannel)
    end
  ensure
    etime = Time.now
    puts "Time: #{etime - stime}s, loading: #{loader_time - stime}, executing: #{etime - loader_time}" if timing
  end
end