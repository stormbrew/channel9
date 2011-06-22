def zero
  yield
end

zero do
  puts "x"
end

def meth
  yield 1
  yield 2
  yield 3
end

meth do |i|
  puts i
end

def meth2
  yield 1,2
  yield 4,5
  yield 5,6
end

meth2 do |a,b|
  puts a
  puts "="
  puts b
end
meth2 do |i|
  puts i
  puts "="
end

meth do |i|
  puts i
  next
  puts "boom"
end
