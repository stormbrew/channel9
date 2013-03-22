$cnt = 0
class X
  attr :readable
  attr :writeable, true

  attr_reader :first_r, :second_r, :third_r
  attr_writer :first_w, :second_w, :third_w
  attr_accessor :first_a, :second_a, :third_a

  def initialize
    @readable = ($cnt += 1)
    @first_r = ($cnt += 1)
    @second_r = ($cnt += 1)
    @third_r = ($cnt += 1)
  end
end

3.times do
  v = X.new
  puts v.readable, v.first_r, v.second_r, v.third_r
  
  v.writeable = "boom"
  puts v.writeable

  v.first_w = "blah"
  v.second_w = "blorp"
  v.third_w = "woop"

  puts v.instance_variable_get(:@first_w), v.instance_variable_get(:@second_w), 
    v.instance_variable_get(:@third_w)

  v.first_a = "stuff"
  v.second_a = "storff"
  v.third_a = "woop"
end

class Y
  class <<self
    attr_accessor :boom
  end
  self.boom = 1
  puts self.boom
end