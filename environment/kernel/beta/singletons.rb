class NilClass
  def nil?
    super
    true
  end
  def to_s
    ""
  end
  def to_i
    0
  end
  def to_proc_prim
    nil
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
end

class UndefClass
  def nil?
    true
  end
  def to_s
    ""
  end
end
