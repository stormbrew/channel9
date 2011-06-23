class Module
  def attr_reader(*names)
    names.each do |name|
      define_method(name) do
        instance_variable_get(:"@#{name}")
      end
    end
  end
  def attr_writer(*names)
    names.each do |name|
      define_method(:"#{name}=") do |val|
        instance_variable_set(:"@#{name}", val)
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

  def module_function(*names)
    # TODO: Implement.
  end
  def alias_method(from, to)
    # TODO: Implement.
  end
  def private_class_method(*names)
    # TODO: Implement.
  end
  def private(*names)
    # TODO: Implement.
  end
  def protected(*names)
    # TODO: Implement.
  end
  def module_eval(ev)
    # TODO: Implement (how?).
  end
end