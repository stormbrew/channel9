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
end