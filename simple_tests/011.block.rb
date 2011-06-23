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

def meth_a
  yield [1,2]
  yield [3,4]
  yield [5,6]
end

meth_a do |a,b|
  puts a
  puts "="
  puts b
end
meth_a do |i|
  puts i
  puts "="
end


meth do |i|
  puts i
  next
  puts "boom"
end

def save(&blah)
  blah
end
def call(&blah)
  blah.call(1)
end
saved = save do |a|
  puts a
end
saved.call(2)
call(&saved)

puts "==="
$z = 0
meth do |i|
  retry if ($z += 1) < 2
  puts i, $z
end
