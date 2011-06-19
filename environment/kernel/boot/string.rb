class String
  def initialize(str)
    @str = str.to_s_prim
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
    @str == other.to_s_prim
  end
  def eql?(other)
    @str.eql?(other.to_s_prim)
  end
  def ===(other)
    @str == other.to_s_prim
  end
end