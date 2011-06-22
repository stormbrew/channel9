class Object
  def class_variable_get(name)
    self.class.class_variable_get(name)
  end
  def class_variable_set(name, val)
    self.class.class_variable_set(name, val)
  end
end