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
  def chr
    "%c" % self
  end
  def size
    # TODO: Make this smarter
    8
  end
  def __c9_object_id__
    self + self + 1 # turns whole number sequence into odd number sequence.
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
    if (o == 0 || o == 0.0)
      raise ZeroDivisionError.new("Divide by Zero")
    end
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

  def succ
    self + 1
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
