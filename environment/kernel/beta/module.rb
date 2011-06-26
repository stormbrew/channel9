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

  def class_eval(s = nil, &block)
    if (s)
      raise NotImplementedError, "String eval not implemented."
    else
      define_singleton_method(:__class_eval__, &block)
      __class_eval__
    end
  end

  def module_function(*names)
    # TODO: Implement.
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
  def module_eval(ev)
    # TODO: Implement (how?).
  end
end