module Kernel
  def sprintf(fmt, *vals)
    fmt = fmt.to_s_prim
    vals = vals.collect do |val|
      if (val.kind_of? String)
        val.to_s_prim
      else
        val
      end
    end
    Channel9::prim_sprintf(fmt, *vals).to_s
  end

  def require(name)
    if ($LOADED_FEATURES.include?(name))
      return false
    end
    begin
      load(name)
      $LOADED_FEATURES << name
      return true
    rescue LoadError
      load(name + ".rb")
      $LOADED_FEATURES << name
      return true
    end
  end

  def kind_of?(other)
    if (other.class == Class)
      # find our own class in the hierarchy of the other.
      klass = self.class
      while (klass)
        if (klass == other)
          return true
        end
        klass = klass.superclass
      end
    end
    return false
  end
  alias_method :is_a?, :kind_of?

  def to_s
    "#<#{self.class}:" + "0x%0#{1.size}x" % object_id
  end

  def at_exit(&block)
    # TODO: Make not a stub.
  end

  def lambda(&block)
    Proc.new(&block)
  end
  def proc(&block)
    Proc.new(&block)
  end

  def Array(ary)
    Array.new(ary.to_tuple_prim)
  end
  def String(s)
    String.new(s.to_s_prim)
  end
  def Integer(i)
    i.to_i
  end

  def instance_eval(s = nil, &block)
    if (s)
      raise NotImplementedError, "String eval not implemented."
    else
      define_singleton_method(:__instance_eval__, &block)
      __instance_eval__
    end
  end
end