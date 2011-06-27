class Proc
  def self.new_from_prim(p)
    a = allocate
    a.initialize(p)
    a
  end
  def self.new(&p)
    p
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
  def to_proc
    self
  end
end

class UnboundMethod
  def initialize(name, prim)
    @name = name
    @p = prim
  end

  def bind(obj)
    Method.new(obj, @name, @p)
  end
end

class Method
  def initialize(obj, name, prim)
    @obj = obj
    @name = name
    @p = prim
  end

  def call(*args)
    @obj.instance_exec(*args, &Proc.new_from_prim(@p))
  end
end