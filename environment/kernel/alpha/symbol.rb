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
  def to_proc
    proc { |obj, *args| obj.send(self, *args) }
  end
end