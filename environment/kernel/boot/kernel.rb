module Kernel
  def require(name)
    load(name + ".rb")
  end

  def method_missing(name) #, *args
    raise NoMethodError, "undefined method `#{name}' for #{to_s}:#{self.class}"
  end

  def puts(*args)
    args.each {|arg|
      if (arg.respond_to?(:each))
        puts(*arg)
      else
        print arg, "\n"
      end
    }
  end

  def to_tuple_prim
    to_a.to_tuple_prim
  end

  def inspect
    to_s
  end
end