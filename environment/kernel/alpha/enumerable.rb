module Enumerable
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

  def include?(val)
    each do |i|
      return true if val == i
    end
    return false
  end
end