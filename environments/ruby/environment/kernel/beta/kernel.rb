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

  def class
    __c9_class__
  end

  def _raw_load_or_compile(name, compile)
    if (name.to_s[0] == :'/'.to_chr)
      if ($__c9_loader.load(name, compile))
        return true
      end
    else
      lp = $LOAD_PATH
      i = lp.length - 1
      name = name.to_s_prim
      "loading: #{name}"
      while (i >= 0)
        path = lp[i].to_s_prim
        if ($__c9_loader.load(path + "/" + name, compile))
          return true
        end
        i -= 1
      end
    end
    raise LoadError, "Could not load library #{name}"
  end
  def load(name)
    _raw_load_or_compile(name, false)
  end
  def load_c9(name)
    $__c9_loader.load_c9(name.to_s_prim)
  end

  def raise(exc, desc = nil, bt = nil)
    if (!exc)
      exc = $!
      if (!exc)
        exc = RuntimeError
      end
    end
    $__c9_unwinder.no_runtime_error(:"No RuntimeError class to raise.") if (!exc)
    if (exc.kind_of?(String) || exc.kind_of?(Symbol))
      bt = desc
      desc = exc
      exc = RuntimeError
    end
    if (!bt)
      bt = $__c9_loader.backtrace.to_a
    end
    if (desc)
      exc = exc.exception(desc)
    else
      exc = exc.exception()
    end
    $! = exc
    $!.set_backtrace(bt)
    $__c9_unwinder.raise($!)
  end

  def caller
    $__c9_loader.backtrace.to_a
  end

  def method_missing(name, *args)
    raise NoMethodError, "undefined method `" + name.to_s + "' for " + to_s + ":" + self.class.to_s
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
  alias is_a? kind_of?

  def print(*args)
    args.each {|text|
      text_prim = text.to_s_prim
      $__c9_ffi_write.call(1, text_prim, text_prim.length)
    }
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

  def exit(n)
    $__c9_exit.call(n)
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

  def to_s_prim
    to_s.to_s_prim
  end

  def inspect
    to_s
  end

  def dup
    nobj = __c9_dup__
    nobj.initialize_copy(self)
    nobj
  end
  def initialize_copy(other)
  end

  def __id__
    __c9_object_id__
  end
  def object_id
    __c9_object_id__
  end

  def respond_to?(name)
    __c9_class__.__c9_lookup__(name.to_s_prim.to_message_id) != undefined
  end

  def hash
    object_id.hash
  end
end
