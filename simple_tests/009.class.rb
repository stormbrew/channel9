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

x = X.new
class <<x
  def bloop
    puts "woot"
  end
end
x.bloop