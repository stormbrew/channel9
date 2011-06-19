begin
  raise "hello"
rescue
  puts $!.message
end

puts "stuff"

x = begin
  raise "what"
rescue
  puts $!.message
  "sam"
ensure
  puts "the"
end

puts x

puts "hell"

class Stuff < StandardError; end

begin
  raise Stuff, "woot"
rescue Stuff => e
  puts e.message
end

begin
  raise "what"
rescue Stuff
  puts "nooo"
rescue RuntimeError
  puts "zoop"
rescue
  puts "hah"
end

begin
  begin
    raise Stuff, "what"
  rescue
    puts "say"
    raise
  end
rescue Stuff
  puts $!.message
end