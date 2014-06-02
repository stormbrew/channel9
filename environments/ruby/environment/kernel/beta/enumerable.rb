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

  def min
    lowest = nil
    if block_given?
      each do |i|
        if !lowest || yield(i, lowest) < 0
          lowest = i
        end
      end
    else
      each do |i|
        if !lowest || i < lowest
          lowest = i
        end
      end
    end
    return lowest
  end
  def max
    highest = nil
    if block_given?
      each do |i|
        if !highest || yield(i, highest) > 0
          highest = i
        end
      end
    else
      each do |i|
        if !highest || i > highest
          highest = i
        end
      end
    end
    highest
  end

  def none?
    if block_given?
      each do |i|
        if yield(i)
          return false
        end
      end
    else
      each do |i|
        if i
          return false
        end
      end
    end
    true
  end

  def one?
    found = false
    if block_given?
      each do |i|
        if yield(i)
          if found
            return false
          end
          found = true
        end
      end
    else
      each do |i|
        if i
          if found
            return false
          end
          found = true
        end
      end
    end
    return found
  end

  def count(cmp = undefined)
    count = 0
    if block_given?
      each do |i|
        count += 1 if yield(i)
      end
    elsif cmp != undefined
      each do |i|
        count += 1 if i == cmp
      end
    else
      each do |i|
        count += 1
      end
    end
    count
  end
end
