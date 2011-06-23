class Array
  include Enumerable

  def initialize(ary)
    @tuple = ary.to_tuple_prim
  end

  def to_tuple_prim
    @tuple
  end
  def to_a
    self
  end

  def to_s
    join
  end

  def join(sep = "")
    r = ""
    l = length
    each do |c|
      r << c
      r << sep if (l -= 1) > 0
    end
    r
  end

  def each
    return if @tuple.nil? #only meaningful in debug output for initialize.
    i = 0
    while (i < @tuple.length)
      yield @tuple.at(i)
      i += 1
    end
  end
  def length
    return 0 if @tuple.nil?
    return @tuple.length
  end

  def split(by)
    Array.new(@tuple.split(by))
  end

  def reverse
    r = []
    i = length - 1
    while (i >= 0)
      r << self[i]
      i -= 1
    end
    r
  end

  def push(val)
    @tuple = @tuple.push(val)
    val
  end
  def <<(val)
    @tuple = @tuple.push(val)
    self
  end
  def unshift(val)
    @tuple = @tuple.front_push(val)
    val
  end
  def pop
    l = @tuple.last
    @tuple = @tuple.pop
    l
  end
  def shift
    l = @tuple.first
    @tuple = @tuple.front_pop
    l
  end
  def [](idx)
    @tuple.at(idx)
  end
  def []=(idx, val)
    @tuple = @tuple.replace(idx, val)
  end

  def +(other)
    Array.new(@tuple + other.to_tuple_prim)
  end
  def -(other)
    n = []
    each do |i|
      n << i if !other.include?(i)
    end
    n
  end
end