class X
  def initialize(x)
    @x = x
  end

  puts "hi!"
  def stuff(a)
    puts @x, a
  end
end
X.new(1).stuff(2)

puts X.new(1).is_a? X
puts X.new(1).is_a? Object
puts X.new(1).is_a? Array

class Y < X
  def stuff(a)
    puts "sub", @x, a
  end
end
Y.new(2).stuff(3)

puts Y.new(2).is_a? Y
puts Y.new(2).is_a? X
puts Y.new(2).is_a? Object
puts Y.new(2).is_a? Array

x = X.new(2)
class <<x
  def bloop
    puts "woot"
  end
end
x.bloop
