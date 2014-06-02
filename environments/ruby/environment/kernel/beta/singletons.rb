class NilClass
  def nil?
    super
    true
  end
  def to_s
    ""
  end
  def inspect
    "nil"
  end
  def to_i
    0
  end
  def to_proc_prim
    nil
  end
  def hash
    0.hash
  end
end

class TrueClass
  def to_s
    "true"
  end
  def &(o)
    if (o)
      false
    else
      true
    end
  end
  def ^(o)
    !o
  end
  def |(o)
    true
  end
  def hash
    2.hash
  end
end

class FalseClass
  def to_s
    "false"
  end
  def &(o)
    false
  end
  def ^(o)
    if (o)
      true
    else
      false
    end
  end
  def |(o)
    if (o)
      true
    else
      false
    end
  end
  def hash
    1.hash
  end
end

class UndefClass
  def nil?
    true
  end
  def to_s
    ""
  end
  def to_i
    0
  end
  def hash
    3.hash
  end
end
