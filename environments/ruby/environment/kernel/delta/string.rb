class String
  include Comparable

  def %(vals)
    sprintf(self, *vals)
  end

  def match(pattern, pos)
    if (!pattern.is_a? Regexp)
      pattern = Regexp.new(pattern)
    end
    pattern.match(self, pos)
  end

  def gsub(pattern, replacement, &block)
    if (!pattern.is_a? Regexp)
      pattern = Regexp.new(pattern)
    end
    pattern.c9_gsub(self, replacement, &block)
  end
end
