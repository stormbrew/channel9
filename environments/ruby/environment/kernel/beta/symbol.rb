class Symbol
  def to_sym
    self
  end
  def to_primitive_str
    self
  end
  def to_s_prim
    self
  end
  def to_s
    String.new(self)
  end
  def to_str
    String.new(self)
  end
  def to_proc
    proc { |obj, *args| obj.send(self, *args) }
  end

  def __c9_object_id__
    __c9_primitive_id__
  end

  def substr(first, last)
    first = length + first if (first < 0)
    last = length + last if (last < 0)
    first = length - 1 if first >= length
    last = length - 1 if last >= length
    if (last < first)
      first, last = last, first
    end
    raise ArgumentError.new("Invalid argument to substr.") if (first < 0 || last < 0)
    substr(first, last)
  end

  def +(other)
    self + other.to_s_prim
  end
end
