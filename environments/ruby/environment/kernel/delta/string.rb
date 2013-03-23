class String
  include Comparable

  def %(vals)
    sprintf(self, *vals)
  end

  def match(*args)
    # TODO: Make not a stub (soon).
    nil
  end
end