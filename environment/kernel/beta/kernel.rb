module Kernel
  def require(name)
    if ($LOADED_FEATURES.include?(name))
      return false
    end
    begin
      load(name)
      $LOADED_FEATURES << name
      return true
    rescue LoadError
      load(name + ".rb")
      $LOADED_FEATURES << name
      return true
    end
  end

  def at_exit(&block)
    # TODO: Make not a stub.
  end
end