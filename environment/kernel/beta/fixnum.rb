class Fixnum
  def ===(other)
#    if (other.class == Fixnum)
 #     if (other == self)
  #      return true
   #   end
    #end
    return false
  end

  def times
    i = 0
    while (i < self)
      i += 1
      yield
    end
  end
end