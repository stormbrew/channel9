class Array
  include Enumerable

  def initialize(ary)
    @tuple = ary.to_tuple_prim
  end

  def to_tuple_prim
    @tuple
  end

  def to_s
    join
  end

  def join(sep = "")
    r = ""
    each do |i|
      r << i
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

  def push(val)
    @tuple = @tuple.push(val)
    val
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
    Array.new(@tuple + other.tuple)
  end
end