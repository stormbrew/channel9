class Float < Numeric
  def to_i
    to_num_primitive
  end
  def to_f
    self
  end
  def to_s
    to_string_primitive.to_s
  end
  def size
    # TODO: Make this smarter
    8
  end
  def ===(other)
    if (other.class == Float)
      if (other == self)
        return true
      end
    end
    return false
  end

  def -@
    negate
  end

  # these are fall-backs for when o isn't a float.
  def <(o)
    self < o.to_f
  end
  def >(o)
    self > o.to_f
  end
  def <=(o)
    self <= o.to_f
  end
  def >=(o)
    self >= o.to_f
  end
  def +(o)
    self + o.to_f
  end
  def -(o)
    self - o.to_f
  end
  def *(o)
    self * o.to_f
  end
  def /(o)
    if (o == 0.0 || o == 0)
      raise ZeroDivisionError.new("Divide by zero")
    end
    self / o.to_f
  end
end