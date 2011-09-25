#!/usr/bin/env ruby
failed = false
total_c9_time = 0.0
total_rb_time = 0.0

Dir["simple_tests/*.rb"].each do |test|
  if (match = test.match(%r{^simple_tests/([0-9]{3,3})\.(.+)\.rb$}))
    num = match[1]
    name = match[2]

    stime = Time.now
    output = `ruby -rubygems -Ilib -I../channel9/lib bin/c9.rb #{test}`
    c9_time = Time.now - stime
    total_c9_time += c9_time

    stime = Time.now
    expected = `ruby #{test} 2>/dev/null`
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