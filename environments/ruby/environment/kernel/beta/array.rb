class Array
  include Enumerable

  def self.new_from_tuple(tuple)
    ary = allocate
    ary.__setup__(tuple, tuple.length)
    ary
  end

  def __setup__(tuple, len)
    @tuple = tuple
    @count = len
  end

  def initialize(ary)
    if (ary.kind_of?(Fixnum))
      count = ary
      ary = [].to_tuple_prim
      count.times do
        ary = ary.push(nil)
      end
      __setup__(ary, count)
    else
      __setup__(ary.to_tuple_prim, ary.length)
    end
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

  # Implementation taken from rubinius/kernel/common/array.rb.
  def sort!
    stack = [[0, length - 1]]

    until stack.empty?
      left, right = stack.pop

      if right > left
        pivotindex = left + ((right - left) / 2)
        # inline pivot routine

        pivot = at(pivotindex)
        swap_elem(pivotindex, right)

        store = left

        i = left
        while i < right
          cmp = (at(i) <=> pivot)
          if cmp < 0
            swap_elem(i, store)
            store += 1
          end

          i += 1
        end

        swap_elem(store, right)

        pi_new = store

        # end pivot
        stack.push [left, pi_new - 1]
        stack.push [pi_new + 1, right]
      end
    end

    self
  end
  def swap_elem(x,y)
    a, b = at(x), at(y)
    @tuple = @tuple.replace(x, b).replace(y, a)
  end
  def sort
    dup.sort!
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

  def c9_inner_flatten(level, from, to)
    if !level || level > 0
      from.each do |i|
        if (i.respond_to? :each)
          c9_inner_flatten(level ? level - 1 : nil, i, to)
        else
          to << i
        end
      end
    end
    to
  end

  def flatten(level = nil)
    c9_inner_flatten(level, self, [])
  end

  def each
    return if @tuple.nil? #only meaningful in debug output for initialize.
    i = 0
    while (i < @tuple.length)
      yield @tuple.at(i)
      i += 1
    end
  end
  def reverse_each
    return if @tuple.nil? or @tuple.length == 0
    i = @tuple.length
    while (i > 0)
      i -= 1
      yield @tuple.at(i)
    end
  end
  def each_with_index
    i = 0
    while (i < @tuple.length)
      yield @tuple.at(i), i
      i += 1
    end
  end
  def length
    return 0 if @tuple.nil?
    return @tuple.length
  end
  def size
    return length
  end
  def empty?
    length == 0
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

  def push(*vals)
    vals.each do |val|
      @tuple = @tuple.push(val)
    end
    vals
  end
  def <<(val)
    @tuple = @tuple.push(val)
    self
  end
  def unshift(*vals)
    vals.each do |val|
      @tuple = @tuple.front_push(val)
    end
    vals
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
  def delete(obj)
    i = 0
    dels = []
    while (i < @tuple.length)
      if (block_given?)
        dels.unshift(i) if yield(@tuple.at(i))
      else
        dels.unshift(i) if (@tuple.at(i) == obj)
      end
      i += 1
    end
    dels.each do |i|
      @tuple = @tuple.delete(i)
    end

    if (dels.length > 0)
      return obj
    else
      return nil
    end
  end
  def delete_at(idx)
    @tuple = @tuple.delete(idx)
    self
  end
  def clear
    @tuple = [].to_tuple_prim
    self
  end
  def reject(obj = nil)
    tuple = [].to_tuple_prim
    if (block_given?)
      each do |i|
        tuple = tuple.push(i) if !yield(i)
      end
    else
      each do |i|
        tuple = tuple.push(i) if i != obj
      end
    end
    Array.new(tuple)
  end
  def reject!(obj = nil, &b)
    @tuple = reject(obj, &b).to_tuple_prim
    self
  end
  def compact
    reject(nil)
  end

  def at(idx, len = nil)
    if (len.nil?)
      @tuple.at(idx)
    else
      @tuple.subary(idx, idx + len).to_a
    end
  end
  def [](idx,len=nil)
    at(idx,len)
  end
  def first
    @tuple.at(0)
  end
  def last
    @tuple.at(-1)
  end
  def []=(idx, val)
    @tuple = @tuple.replace(idx, val)
    val
  end

  def +(other)
    Array.new(@tuple + other.to_tuple_prim)
  end
  def concat(other)
    @tuple = @tuple + other.to_tuple_prim
    self
  end
  def -(other)
    n = []
    each do |i|
      n << i if !other.include?(i)
    end
    n
  end

  def ==(o)
    if (o.is_a?(Array) && o.length == length)
      l = length
      i = 0
      while i != l
        return false if o.at(i) != at(i)
        i += 1
      end
      return true
    end
    return false
  end
end
