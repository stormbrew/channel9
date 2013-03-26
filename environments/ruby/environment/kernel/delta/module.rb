class Module
  def class_variable_decl(name, val)
    @class_variables ||= $__c9_BasicHash.call($__c9_prime_gen)
    @class_variables.set(name.to_s_prim, val)
  end
  def class_variable_get(name)
    if (@class_variables && (cvar = @class_variables.get(name.to_s_prim)) != undefined)
      cvar
    elsif (superclass)
      superclass.class_variable_get(name)
    else
      raise NameError, "uninitialized class variable #{name}"
    end
  end
  def class_variable_set(name, val)
    name = name.to_s_prim
    if (@class_variables && (cvar = @class_variables.get(name)) != undefined)
      @class_variables.set(name, val)
    elsif (superclass)
      superclass.class_variable_set(name, val)
    else
      raise NameError, "uninitialized class variable #{name}"
    end
  end

  def attr_reader(*names)
    names.each do |name|
      define_method(name) do
        __c9_ivar_get__(:"@#{name}")
      end
    end
  end
  def attr_writer(*names)
    names.each do |name|
      define_method(:"#{name}=") do |val|
        __c9_ivar_set__(:"@#{name}", val)
      end
    end
  end
  def attr_accessor(*names)
    attr_reader(*names)
    attr_writer(*names)
  end
  def attr(name, write = false)
    attr_reader(name)
    attr_writer(name) if write
  end

  def class_eval(s = nil, &block)
    if (s)
      block = Channel9.compile_string(:eval, s, "__eval__", 1)
      if (!block)
        raise ParseError
      end
    end
    cscope = [self,
      [::Object, nil].to_tuple_prim
    ].to_tuple_prim
    __c9_instance_eval__(cscope, &block)
  end
  alias_method :module_eval, :class_eval

  def instance_method(name)
    if (m = __c9_lookup__(name.to_sym.to_message_id))
      UnboundMethod.new(name.to_sym, m)
    else
      nil
    end
  end

  def module_function(*names)
    names.each do |name|
      id = name.to_sym.to_message_id
      if (m = __c9_lookup__(id))
        __c9_make_singleton__.__c9_add_method__(id, m)
      else
        raise "Unknown method #{name}"
      end
    end
  end

  def public_class_method(*names)
    # TODO: Implement.
  end
  def protected_class_method(*names)
    # TODO: Implement.
  end
  def private_class_method(*names)
    # TODO: Implement.
  end
  def public(*names)
    # TODO: Implement.
  end
  def private(*names)
    # TODO: Implement.
  end
  def protected(*names)
    # TODO: Implement.
  end
end
