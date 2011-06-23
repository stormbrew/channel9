class Hash
  def initialize(tuple)
    @vals = []
    key = nil
    tuple.each do |i|
      if (key)
        @vals.push([key, i])
      else
        key = i
      end
    end
  end

  def []=(name, val)
    @vals.each do |a|
      if (name.eql?(a[0]))
        a[1] = val
      end
    end
    @vals.push([name, val])
  end
  def [](name)
    @vals.each do |k,v|
      if (name.eql?(k))
        return v
      end
    end
    nil
  end
  def include?(name)
    @vals.each do |k,v|
      if (name.eql?(k))
        return true
      end
      false
    end
  end
end