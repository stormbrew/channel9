class Class
  def ===(other)
    # TODO: This is naive. Needs to check inheritence.
    if (other.class == Class)
      self == other
    else
      self == other.class
    end
  end
end