a = "blah"
b = a.dup

puts(a)
puts(b)
puts(a.object_id == b.object_id)

class X
  attr :a
  attr :b
  def initialize()
    @a = 1
  end
  def initialize_copy(other)
    @a += 1
    @b = 2
  end
end

a = X.new
puts a.a, a.b
b = a.dup
puts a.a, a.b
puts b.a, b.b