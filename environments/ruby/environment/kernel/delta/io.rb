class IO
  def self.new
    raise NotImplementedError, "IO.new not implemented."
  end

  def self.readlines(filename)
    lines = []
    File.open(filename, "r") do |f|
      f.each_line do |line|
        lines << line
      end
    end
    lines
  end

  def write
    raise NotImplementedError, "IO#write not implemented."
  end
  def print(*args)
    write(args.join(""))
  end
  def <<(*args)
    write(args.join(""))
  end

  def puts(*args)
    args.each {|arg|
      if (arg.kind_of?(::Channel9::Tuple) || arg.kind_of?(::StaticTuple) || arg.kind_of?(::Array))
        puts(*arg)
      elsif (arg.nil?)
        write(:"nil\n")
      else
        raw = arg.to_s_prim
        len = raw.length
        if len > 0 && raw.substr(len-1,len-1) == :"\n"
          write(raw)
        else
          write(raw + :"\n")
        end
      end
    }
  end

  def gets(*args)
    raise NotImplementedError, "IO#gets not implemented."
  end

  def read
    raise NotImplementedError, "IO#read not implemented."
  end

  def each_line
    while l = gets
      yield l
    end
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
