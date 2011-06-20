class Tuple
  def to_tuple_prim
    self
  end
  def to_a
    Array.new(self)
  end
end