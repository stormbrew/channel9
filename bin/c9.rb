#!/usr/bin/env ruby
if $0 =~ %r{bin/c9.rb$}
  $LOAD_PATH.unshift "../channel9/ruby/lib"
  $LOAD_PATH.unshift "../channel9.rb/lib"
  require 'rubygems'
end

require 'fileutils'
require 'channel9'
require 'channel9/loader/ruby'

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
loader = Channel9::Loader::Ruby.new
if (print)
  stream = Channel9::Loader::Ruby.compile(args.shift)
  puts stream.to_json
else
  infile = args.shift
  stream = Channel9::Loader::Ruby.compile(infile)
  if !(outfile = args.shift)
    outfile = infile.gsub(%r{/([^/]+?)(\.rb)?$}) do |m|
      "/#{$1}.c9b"
    end
  end
  puts File.dirname(outfile)
  FileUtils.mkdir_p(File.dirname(outfile))
  File.open(outfile, "w") do |f|
    f.write stream.to_json
  end
end
