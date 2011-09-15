class Class
  def ===(other)
    if (other.class == Class)
      self == other
    else
      other.kind_of?(self)
    end
  end

  def class_variable_decl(name, val)
    @class_variables ||= {}
    @class_variables[name] = val
  end
  def class_variable_get(name)
    if (@class_variables && @class_variables.include?(name))
      @class_variables[name]
    elsif (superclass)
      superclass.class_variable_get(name)
    else
      raise NameError, "uninitialized class variable #{name}"
    end
  end
  def class_variable_set(name, val)
    if (@class_variables && @class_variables.include?(name))
      @class_variables[name] = val
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