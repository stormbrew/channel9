class StaticTuple
  include Enumerable

  def to_tuple_prim
    self
  end

  def to_a
    Array.new_from_tuple(self)
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
    if (length > 0 && pos < 0)
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

  def hash
    # TODO: This is a terrible hash function. FIXME
    h = 0xf51afd7ed558ccd
    i = 0
    while (i < length)
      h ^= at(i)
      i += 1
    end
    h
  end

  def subary(b, e)
    if (b > e)
      return subary(e, b)
    elsif (e > length)
      return subary(b, length)
    end
    changed = false
    if (b < 0)
      changed = true
      b = length - b
    end
    if (e < 0)
      changed = true
      e = length - e
    end
    raise ArgumentError, "Invalid arguments to StaticTuple#subary (length: #{length}, args: (#{b}, #{e}))" if !changed
    subary(b, e)
  end
end
