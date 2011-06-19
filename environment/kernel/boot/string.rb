class String
  def initialize(str)
    @str = str.to_primitive_str
  end

  def to_sym
    @str
  end
  def to_primitive_str
    @str
  end
  def to_s_prim
    @str
  end
  def to_s
    self
  end

  def ==(other)
    @str == other.to_primitive_str
  end
  def eql?(other)
    @str.eql?(other.to_primitive_str)
  end
  def ===(other)
    @str == other.to_primitive_str
  end
end