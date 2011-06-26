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
    $LOAD_PATH.reverse.each {|path|
      if (raw_load("#{path}/#{name}"))
        return true
      end
    }
    raise LoadError, "Could not load library #{name}"
  end

  def method_missing(name, *args)
    raise NoMethodError, "undefined method `#{name}' for #{to_s}:#{self.class}"
  end

  def puts(*args)
    args.each {|arg|
      if (arg.kind_of?(Tuple) || arg.kind_of?(Array))
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