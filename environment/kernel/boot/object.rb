class Object
  def nil?
    false
  end

  def ==(other)
    self.equal?(other)
  end

  def eql?(other)
    self.equal?(other)
  end

  def ===(other)
    self.equal?(other)
  end
end