module Comparable
  def <(other)
    (self <=> other) < 0
  end
  def <=(other)
    (self <=> other) <= 0
  end
  def >(other)
    (self <=> other) > 0
  end
  def >=(other)
    (self <=> other) >= 0
  end 

  def between?(min, max)
    ((self <=> min) >= 0) && ((self <=> max) <= 0)
  end
end