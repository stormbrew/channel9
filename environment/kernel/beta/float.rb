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
end