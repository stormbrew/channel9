class Hash
  include Enumerable

  attr :default, true
  attr :default_proc, true

  def self.new_from_tuple(tuple)
    a = allocate
    a.initialize(tuple, nil, nil)
    a
  end
  def self.new(default, &default_proc)
    a = allocate
    a.initialize(nil, default, default_proc)
    a
  end
  def initialize(tuple, default, default_proc)
    @vals = []
    if (tuple)
      key = nil
      tuple.each do |i|
        if (key)
          @vals.push([key, i])
          key = nil
        else
          key = i
        end
      end
    end
    @default = default
    @default_proc = default_proc
  end

  def each
    @vals.each do |k,v|
      yield k,v
    end
  end
  alias_method :each_pair, :each

  def []=(name, val)
    @vals.each do |a|
      if (name.eql?(a[0]))
        a[1] = val
      end
    end
    @vals.push([name, val])
    val
  end
  def [](name)
    @vals.each do |k,v|
      if (name.eql?(k))
        return v
      end
    end
    if (@default_proc)
      @default_proc.call(self, name)
    else
      @default
    end
  end
  def keys
    @vals.collect {|k,v| k }
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