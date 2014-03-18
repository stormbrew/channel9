#!/usr/bin/env ruby
require 'rbconfig'

failed = false
total_c9_time = 0.0
total_rb_time = 0.0

dir = ARGV.first
if !dir || !File.directory?(dir)
  raise "No directory specified or directory not found (#{dir})."
end

if ENV['C9RB_PATH']
  C9RB_PATH = ENV['C9RB_PATH']
else
  # add a likely location for the cmake files to the path
  # so this is more likely to work
  ENV['PATH'] = ENV['PATH'] + ':../channel9.build/bin'
  C9RB_PATH = 'c9.rb'
end

Dir["#{dir}/*.rb"].sort.each do |test|
  if (match = test.match(%r{^#{dir}/([0-9]{3,3})\.(.+)\.rb$}))
    num = match[1]
    name = match[2]

    stime = Time.now
    output = `#{C9RB_PATH} #{test}`
    c9_time = Time.now - stime
    total_c9_time += c9_time

    stime = Time.now
    expected = `#{RbConfig::CONFIG['bindir']}/#{RbConfig::CONFIG['ruby_install_name']} #{test} 2>/dev/null`
    rb_time = Time.now - stime
    total_rb_time += rb_time

    ok = output == expected
    puts("#{num} #{name}: #{ok ? 'OK' : 'FAIL'} rb:#{rb_time}s, c9:#{c9_time}s")

    if (!ok)
      failed = true
      puts "Expected:"
      puts "\t#{expected.gsub("\n", "\n\t")}"
      puts "Got:"
      puts "\t#{output.gsub("\n", "\n\t")}"
    end
  end
end

puts("---")
puts("total rb:#{total_rb_time}s")
puts("total c9:#{total_c9_time}s")

exit(1) if failed
