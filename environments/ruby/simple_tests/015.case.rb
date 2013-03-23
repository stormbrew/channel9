case "blah"
when 1
  puts "boom"
when "blah"
  puts "heydar"
end

case "blah"
when 1
  puts "boom"
when String
  puts "stuff"
end

case "blorp"
when 1
  puts "boom"
else
  puts "gump"
end

case
when false
  puts "zorp"
when true
  puts "x"
end

case
when nil
  puts "zoop"
when 1
  puts "y"
end

case
when nil
  puts "zero"
when false
  puts "stuff"
else
  puts "z"
end
