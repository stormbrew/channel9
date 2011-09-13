class Thread
  def self.current
    @cur ||= new
  end
  def self.[](name)
    current[name]
  end
  def self.[]=(name, val)
    current[name] = val
  end

  def initialize
    @tls = {}
  end
  def [](name)
    @tls[name]
  end
  def []=(name, val)
    @tls[name] = val
  end
end