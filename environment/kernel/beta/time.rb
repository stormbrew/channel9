class Time
  include Comparable

  def initialize(ts)
    @ts = ts
  end

  def self.at(ts)
    i = allocate
    i.initialize(ts)
    i
  end

  def self.new
    i = allocate
    i.initialize(Channel9::prim_time_now)
    i
  end
  def self.now
    new
  end

  def to_i
    @ts
  end

  def <=>(other)
    other.kind_of?(Time) && @ts <=> other.to_i
  end
  def eql?(other)
    other.kind_of?(Time) && @ts == other.to_i
  end

  def +(i)
    @ts += i.to_i
  end
  def -(i)
    @ts -= i.to_i
  end
end

