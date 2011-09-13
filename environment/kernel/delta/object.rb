class Object
  def instance_variable_set(name, val)
    __c9_ivar_set__(name.to_sym, val)
  end
  def instance_variable_get(name)
    __c9_ivar_get__(name.to_sym)
  end

  def class_variable_get(name)
    self.class.class_variable_get(name)
  end
  def class_variable_set(name, val)
    self.class.class_variable_set(name, val)
  end

  def extend(mod)
    self.singleton!.include(mod)
  end
  def singleton_methods
    [] # TODO: Implement properly.
  end
  
  def method(name)
    meth = nil
    if (s = singleton)
      meth = s.instance_method(name)
    else
      meth = self.class.instance_method(name)
    end
    if (meth)
      meth.bind(self)
    else
      nil
    end
  end

  def frozen?
    false
  end

  alias_method :__id__, :object_id
end