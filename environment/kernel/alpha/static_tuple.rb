class StaticTuple
  include Enumerable

  def to_tuple_prim
    self
  end
  
  def to_a
    Array.new(self)
  end

  def first
    nil # primitive passes through if no items
  end
  def last
    nil # primitive passes through if no items
  end
  def front_pop
    self # primitive passes through if no items
  end
  def pop
    self # primitive passes through if no items
  end

  def at(pos)
    if (pos < 0)
      at(length + pos)
    else
      nil # asked for an out of range item.
    end
  end

  def each
    i = 0
    while (i < length)
      yield at(i)
      i += 1
    end
  end
end
