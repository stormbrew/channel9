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

module K
  def wat
    puts "zomg"
  end
end

class Y
  include K
end

test.wat
