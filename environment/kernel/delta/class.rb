class Class
  def ===(other)
    if (other.class == Class)
      self == other
    else
      other.kind_of?(self)
    end
  end

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

  def to_s
    name
  end

  def define_method(name, &method)
    __c9_add_method__(name.to_s_prim, method.to_proc_prim)
  end
  def define_singleton_method(name, &method)
    __c9_make_singleton__.__c9_add_method__(name.to_s_prim, method.to_proc_prim)
  end
end