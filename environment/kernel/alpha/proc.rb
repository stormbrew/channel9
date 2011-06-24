class Proc
  def self.new_from_prim(p)
    a = allocate
    a.initialize(p)
    a
  end
  def self.new(&p)
    a = allocate
    a.initialize(p)
    a
  end

  def initialize(p)
    @p = p
  end

  def call(*args)
    @p.call(*args)
  end
  def [](*args)
    @p.call(*args)
  end

  def ===(other)
    @p.call(other)
  end

  def arity
    -1 # TODO: Make this work better.
  end

  def to_proc_prim
    @p
  end
end
