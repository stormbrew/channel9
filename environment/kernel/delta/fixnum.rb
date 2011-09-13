class Numeric
  def blah; end
end

class Fixnum < Numeric
  def to_i
    self
  end
  def to_f
    to_float_primitive
  end
  def to_s
    to_string_primitive.to_s
  end
  def size
    # TODO: Make this smarter
    8
  end
  def ===(other)
    if (other.class == Fixnum)
      if (other == self)
        return true
      end
    end
    return false
  end

  def <=>(other)
    self - other # TODO: normalize to -1, 0, +1
  end
  def **(other)
    self ** other.to_i
  end
  def *(other)
    self * other.to_i
  end
  def +(o)
    self + o.to_i
  end
  def -(o)
    self - o.to_i
  end
  def /(o)
    self / o.to_i
  end
  def %(o)
    self % o.to_i
  end

  def div(other)
    self / other
  end

  def -@
    negate
  end

  def times
    i = 0
    while (i < self)
      i += 1
      yield
    end
  end

  def gcd(o)
    if o == 0
      return self
    else
      return o.gcd(self % o)
    end
  end

  # These are called if the primitive
  # errors for some reason in its default handling
  # of these operators.
  def <(o)
    self < o.to_int
  end
  def >(o)
    self > o.to_int
  end
end