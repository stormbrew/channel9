module Z
  def blah(x, y)
    puts x, y
  end

  def blorp(a, b)
    puts a, b
  end
end

class Y
  include Z

  def blorp(a, b)
    puts "hello"
  end
end

test = Y.new
test.blah(1,2)
test.blorp(3,4)

puts test.is_a? Enumerable
puts test.is_a? Z

module K
  def wat
    puts "zomg"
  end
end

puts test.is_a? K

class Y
  include K
end

puts test.is_a? K

test.wat

class T < Y
end

puts Y.new.is_a? Z
puts Y.new.is_a? K
