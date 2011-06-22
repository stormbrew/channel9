module Enumerable
  def collect
    r = []
    each do |i|
      r << yield(i)
    end
    r
  end
end