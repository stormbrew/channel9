module Kernel
  def initialize_copy(other)
  end

  def freeze
  end

  def nil?
    false
  end

  def ==(other)
    self.equal?(other)
  end

  def eql?(other)
    self.equal?(other)
  end

  def ===(other)
    self.equal?(other)
  end

  def =~(other)
    nil
  end

  def load(name)
    lp = $LOAD_PATH
    i = lp.length - 1
    while (i >= 0)
      path = lp[i]
      if (raw_load("#{path}/#{name}".to_s_prim))
        return true
      end
      i -= 1
    end
    raise LoadError, "Could not load library #{name}"
  end

  def method_missing(name, *args)
    raise NoMethodError, "undefined method `#{name}' for #{to_s}:#{self.class}"
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

  def puts(*args)
    args.each {|arg|
      if (arg.kind_of?(::Channel9::Tuple) || arg.kind_of?(::StaticTuple) || arg.kind_of?(::Array))
        puts(*arg)
      elsif (arg.nil?)
        print("nil\n")
      else
        print arg, "\n"
      end
    }
  end

  def exit(n)
    special_channel(:exit).call(n)
  end

  def to_tuple_prim
    to_a.to_tuple_prim
  end
  def to_a
    [self]
  end

  def to_proc_prim
    to_proc.to_proc_prim
  end

  def inspect
    to_s
  end
end