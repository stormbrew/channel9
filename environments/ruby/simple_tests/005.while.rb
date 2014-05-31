i = 0
while i < 5
  puts(i)
  i += 1
end

i = 0
until i == 5
  puts(i)
  i += 1
end

i = 0
while (i += 1) < 5
  ;
end
puts(i)

i = 0
until (i += 1) > 5
  ;
end
puts(i)
