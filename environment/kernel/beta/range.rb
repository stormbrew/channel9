class Range
  include Enumerable

  attr_accessor :begin, :end

  def initialize(s, e, exclusive = false)
    @begin = s
    @end = e
    @exclusive = exclusive
  end

  def exclude_end?
    @exclusive
  end

  def first(n = nil)
    if (n)
      raise NotImplementedError, "Range#first(n)"
    else
      @begin
    end
  end
  def last(n = nil)
    if (n)
      raise NotImplementedError, "Range#last(n)"
    else
      @exclusive ? @end - 1 : @end
    end
  end

  def each
    i = @begin
    while (i != @end)
      yield(i)
      i = i.succ
    end
    if (!@exclusive)
      yield(i)
    end
  end
  def step(n = 1, &block)
    if (n == 1)
      each(&block)
    elsif (@exclusive)
      i = @begin
      while (i < @end)
        yield(i)
        i += n
      end
    else
      i = @begin
      while (i <= @end)
        yield(i)
        i += n
      end
    end
  end     

  def eql?(obj)
    obj.class == Range && 
    obj.begin == self.begin && 
    obj.end == self.end && 
    obj.exclude_end? == exclude_end?
  end
  def ==(obj)
    eql?(obj)
  end

  def cover?(obj)
    if (@exclusive)
      obj >= @begin &&
      obj < @end
    else
      obj >= @begin &&
      obj <= @end
    end
  end
  alias_method :===, :cover?
  alias_method :include?, :cover?
  def hash
    [@begin, @end, @exclusive].hash
  end

  def to_s
    "#{@begin}..#{@exclusive ? '' : '.'}#{@end}"
  end
  def inspect
    "#{@begin.inspect}..#{@exclusive ? '' : '.'}#{@end.inspect}"
  end
end