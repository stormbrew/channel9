class StaticTuple
  include Enumerable

  def to_tuple_prim
    self
  end
  
  def to_a
    Array.new(self)
  end

  def each
    i = 0
    while (i < length)
      yield at(i)
      i += 1
    end
  end
end
