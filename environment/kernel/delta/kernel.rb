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
    buf = $__c9_ffi_sprintf_buf.call()
    if ($__c9_ffi_sprintf.call(buf, fmt, *vals) > 0)
      return buf.call(0).to_s
    else
      raise "System failure in sprintf"
    end
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

#  def to_s
#    "#<#{self.class}:" + object_id + ">" # "0x%0#{1.size}x" % object_id + ">"
#  end

  def at_exit(&block)
    # TODO: Make not a stub.
  end

  def lambda(&block)
    Proc.new_from_prim($__c9_long_return_catcher.send(block.to_proc_prim))
  end
  def proc(&block)
    Proc.new_from_prim($__c9_long_return_catcher.send(block.to_proc_prim))
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
      block = Channel9.compile_string(:eval, s, "__eval__", 1)
      if (!block)
        raise ParseError
      end
    end
    cscope = [__c9_make_singleton__, [
        self.class, [
          [::Object, nil].to_tuple_prim
        ].to_tuple_prim
      ].to_tuple_prim
    ].to_tuple_prim
    __c9_instance_eval__(cscope, &block)
  end
  def eval(s, filename = "__eval__", line = 1)
    # TODO: This needs to at some point get the const-self from the
    # caller. Not sure how that'll work just yet, though.
    proc = Channel9.compile_string(:eval, s, filename, line)
    if (proc)
      $__c9_global_self.instance_eval(&proc)
    else
      raise ParseError
    end
  end
  def instance_exec(*args, &block)
    __c9_instance_eval__(undefined, *args, &block)
  end
  def instance_variables
    instance_variables_prim.to_a
  end

  def send(name, *args, &block)
    __c9_send__(name.to_s_prim, *args, &block)
  end
  alias_method :__send__, :send
end
