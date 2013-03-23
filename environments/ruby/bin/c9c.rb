#!/usr/bin/env ruby
if $0 =~ %r{bin/c9c.rb$}
  $LOAD_PATH.unshift "../channel9/ruby/lib"
  $LOAD_PATH.unshift "../channel9.rb/lib"
  require 'rubygems'
end

require 'fileutils'
require 'channel9'
require 'channel9/ruby'

args = ARGV

print = args.include?("-p")
args.delete("-p")
timing = args.include?("-t")
args.delete("-t")
# trace all, start off tracing before the script itself is run.
if (args.include?("-v"))
  puts("Channel9.rb 0.0.0.0.1")
  args.delete("-v")
end
if (print)
  stream = Channel9::Ruby.compile(args.shift)
  puts stream.to_json
else
  infile = args.shift
  stream = Channel9::Ruby.compile(infile)
  if !(outfile = args.shift)
    outfile = infile + ".c9b"
  end
  puts File.dirname(outfile)
  FileUtils.mkdir_p(File.dirname(outfile))
  File.open(outfile, "w") do |f|
    f.write stream.to_json
  end
end
