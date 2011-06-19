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