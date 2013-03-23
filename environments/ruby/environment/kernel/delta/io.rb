class IO
  def self.new
    raise NotImplementedError, "IO.new not implemented."
  end

  def write
    raise NotImplementedError, "IO#write not implemented."
  end
  def print(*args)
    write(*args)
  end
  def <<(*args)
    write(*args)
  end

  def puts(*args)
    args.each {|arg|
      if (arg.kind_of?(::Channel9::Tuple) || arg.kind_of?(::StaticTuple) || arg.kind_of?(::Array))
        puts(*arg)
      elsif (arg.nil?)
        print(:"nil\n")
      else
        print arg,:"\n"
      end
    }
  end

  def read
    raise NotImplementedError, "IO#read not implemented."
  end

  def flush
  end
end

class Stdout < IO
  def self.new
    s = allocate
    s.initialize()
    s
  end

  def write(*args)
    args.each {|text|
      $__c9_ffi_write.call(1, text.to_s_prim, text.length)
    }
  end
end
class Stderr < IO
  def self.new
    s = allocate
    s.initialize()
    s
  end

  def write(*args)
    args.each {|text|
      $__c9_ffi_write.call(2, text.to_s_prim, text.length)
    }
  end
end

class Stdin < IO
  def self.new
    s = allocate
    s.initialize()
    s
  end
end
