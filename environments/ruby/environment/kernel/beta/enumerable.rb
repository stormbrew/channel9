module Enumerable
  def each_with_index
    n = 0
    each do |i|
      yield(i, n)
      n += 1
    end
  end

  def collect
    r = []
    each do |i|
      r << yield(i)
    end
    r
  end
  def map
    r = []
    each do |i|
      r << yield(i)
    end
    r
  end

  def select
    r = []
    each do |i|
      r << i if yield(i)
    end
    r
  end

  def find(ifnone = nil)
    each do |i|
      if yield(i)
        return i
      end
    end
    return ifnone
  end

  def include?(val)
    each do |i|
      return true if val == i
    end
    return false
  end

  def any?
    if (block_given?)
      each do |i|
        return true if yield(i)
      end
    else
      each do |i|
        return true if i
      end
    end
    return false
  end
  def all?
    if (block_given?)
      each do |i|
        return false if !yield(i)
      end
    else
      each do |i|
        return false if !i
      end
    end
    return true
  end

  def inject(initial = :__NONE__, sym = :__NONE__)
    if (block_given?)
      if (initial == :__NONE__)
        initial = first
      end
      each do |i|
        initial = yield(initial, i)
      end
      return initial
    else
      if (initial == :__NONE__)
        initial = first
      end
      if (sym == :__NONE__)
        raise ArgumentError, "No symbol or block passed in to inject."
      end
      each do |i|
        initial = initial.send(sym, i)
      end
      return initial
    end
  end
end
