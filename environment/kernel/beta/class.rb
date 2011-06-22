class Class
  def ===(other)
    # TODO: This is naive. Needs to check inheritence.
    if (other.class == Class)
      self == other
    else
      self == other.class
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
end