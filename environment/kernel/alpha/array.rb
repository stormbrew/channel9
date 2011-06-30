class Array
  include Enumerable

  def initialize(ary)
    if (ary.class == ::Array)
      @tuple = ary.instance_variable_get(:@tuple).dup
      @start = ary.instance_variable_get(:@start)
      @length = ary.instance_variable_get(:@length)
    elsif (ary.class == ::StaticTuple || ary.class == ::Channel9::Tuple)
      @tuple = ary
      @start = 0
      @length = @tuple.length
    else
      ary ||= 0
      ary = ary.to_int
      @tuple = ::Channel9::Tuple.new(ary + 8)
      @start = 0
      @length = ary
    end
    @read_only = @tuple.class == ::StaticTuple
  end

  def make_writeable
    if (@read_only)
      new_tuple = Channel9::Tuple.new(@length)
      i = 0
      while (i < @length)
        new_tuple.put(i, @tuple.at(@start + i))
        i += 1
      end
      @tuple = new_tuple
      @start = 0
      @read_only = false
    end
  end
  def expand(size)
    if (@start + size > @tuple.length)
      new_tuple = Channel9::Tuple.new(size + (size - @length)*2)
      i = 0
      while (i < @length)
        new_tuple.put(i, @tuple.at(@start + i))
        i += 1
      end
      @tuple = new_tuple
      @start = 0
      @read_only = false
    end
  end

  def to_tuple
    if (@start == 0 && @length == @tuple.length)
      @tuple
    else
      tuple = Channel9::Tuple.new(@length)
      i = 0
      while (i < @length)
        tuple.put(i, @tuple.at(@start + i))
        i += 1
      end
      tuple
    end
  end
  def to_tuple_prim
    to_tuple.to_tuple_prim
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
    make_writeable
    a, b = at(x), at(y)
    @tuple.put(@start + y, a)
    @tuple.put(@start + x, b)
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

  def each
    i = 0
    while (i < @length)
      yield @tuple.at(@start + i)
      i += 1
    end
  end
  def each_with_index
    i = 0
    while (i < @length)
      yield @tuple.at(@start + i), i
      i += 1
    end
  end    
  def length
    @length
  end
  def size
    return length
  end
  def empty?
    length == 0
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
    adding = vals.length
    expand(@length + adding)
    pos = @start + @length
    i = 0
    while (i < adding)
      @tuple.put(pos, vals.at(i))
      pos += 1
      i += 1
    end
    @length = pos - @start
    vals
  end
  def <<(val)
    push(val)
    self
  end
  def unshift(*vals)
    ary = vals + self
    @tuple = ary.to_tuple
    @start = 0
    @length = ary.length
    @read_only = false
  end
  def pop
    @length -= 1
    l = @tuple.at(@start + @length)
    @tuple.put(@start + @length, nil) if !@read_only
    l
  end
  def shift
    l = @tuple.at(@start)
    @tuple.put(@start, nil) if !@read_only
    @start += 1
    l
  end
  def delete(obj)
    new_tuple = Channel9::Tuple.new(@length)
    i = @start
    count = 0
    if (block_given?)
      while (i < @length)
        val = @tuple.at(@start + i)
        if !yield(val)
          new_tuple.put(count, val)
          count += 1
        end
        i += 1
      end
    else
      while (i < @length)
        val = @tuple.at(@start + i)
        if (val != obj)
          new_tuple.put(count, val)
          count += 1
        end
        i += 1
      end
    end
    orig_len = @length
    @tuple = new_tuple
    @start = 0
    @length = count
    @read_only = false

    if (count < orig_len)
      return obj
    else
      return nil
    end
  end
  alias_method :reject, :delete
  def reject!(obj = nil, &b)
    rejected = reject(obj, &b)
    @tuple = rejected.to_tuple
    @start = 0
    @length = rejected.length
    @read_only = false
  end

  def delete_at(idx)
    make_writeable
    i = idx
    while (i < @length)
      @tuple.put(@start + i, @tuple.at(@start + i + 1))
      i += 1
    end
    @tuple.put(@start + @length, nil)
    @length -= 1
  end
  def clear
    @tuple = [].to_tuple_prim
  end
  def compact
    reject(nil)
  end

  def at(idx, len = nil)
    if (len.nil?)
      @tuple.at(@start + idx)
    else
      @tuple.subary(@start + idx, @start + idx + len).to_a
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
    make_writeable
    @tuple.put(idx, val)
  end

  def +(other)
    other_len = other.length
    new_tuple = Channel9::Tuple.new(@length + other_len)
    i = @start
    while (i < @length)
      new_tuple.put(@start + i, @tuple.at(@start + i))
      i += 1
    end
    j = 0
    while (j < other_len)
      new_tuple.put(@start + i, other.at(j))
      i += 1
      j += 1
    end
    Array.new(new_tuple)
  end
  def -(other)
    n = []
    each do |i|
      n << i if !other.include?(i)
    end
    n
  end
end