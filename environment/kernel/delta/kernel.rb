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

  def to_s
    "#<#{self.class}:" + "0x%0#{1.size}x" % object_id + ">"
  end

  def at_exit(&block)
    # TODO: Make not a stub.
  end

  def lambda(&block)
    block
  end
  def proc(&block)
    block
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
    instance_eval_prim(&block)
  end
  def eval(s, filename = "__eval__", line = 1)
    proc = Channel9.compile_string(:eval, s, filename, line)
    if (proc)
      special_channel(:global_self).instance_eval(&proc)
    else
      raise ParseError
    end
  end
  def instance_exec(*args, &block)
    instance_eval_prim(*args, &block)
  end
  def instance_method(name)
    if (method_defined?(name.to_sym))
      UnboundMethod.new(name.to_sym, instance_method_prim(name.to_s_prim))
    else
      nil
    end
  end
  def instance_variables
    instance_variables_prim.to_a
  end

  def send(name, *args, &block)
    send_prim(name.to_s_prim, *args, &block)
  end
  alias_method :__send__, :send
end