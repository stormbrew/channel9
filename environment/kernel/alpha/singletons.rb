class NilClass
  def nil?
    true
  end
  def to_s
    ""
  end
end

class TrueClass
  def to_s
    "true"
  end
end

class FalseClass
  def to_s
    "false"
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
