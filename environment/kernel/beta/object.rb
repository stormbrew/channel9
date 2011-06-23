class Object
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
end