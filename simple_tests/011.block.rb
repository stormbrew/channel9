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
  yield 4,5
end

meth do |i|
  puts i
end
