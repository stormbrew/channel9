#!/usr/bin/env ruby
failed = false

Dir["simple_tests/*.rb"].each do |test|
  if (match = test.match(%r{^simple_tests/([0-9]{3,3})\.(.+)\.rb$}))
    num = match[1]
    name = match[2]

    output = `ruby -rubygems -Ilib -I../channel9/lib bin/c9.rb #{test}`
    expected = `ruby #{test} 2>/dev/null`
    ok = output == expected
    puts("#{num} #{name}: #{ok ? 'OK' : 'FAIL'}")

    if (!ok)
      failed = true
      puts "Expected:"
      puts "\t#{expected.gsub("\n", "\n\t")}"
      puts "Got:"
      puts "\t#{output.gsub("\n", "\n\t")}"
    end
  end
end

exit(1) if failed