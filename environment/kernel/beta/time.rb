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
    @ts.to_i
  end
  def to_f
    @ts.to_f
  end

  def <=>(other)
    other.kind_of?(Time) && @ts <=> other.to_i
  end
  def eql?(other)
    other.kind_of?(Time) && @ts == other.to_i
  end

  def +(i)
    Time.at(@ts + i.to_f)
  end
  def -(i)
    if (i.kind_of?(Time))
      @ts.to_f - i.to_f
    else
      Time.at(@ts - i.to_f)
    end
  end
end

