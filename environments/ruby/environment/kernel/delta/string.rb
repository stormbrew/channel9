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
  def sub(pattern, replacement, &block)
    if (!pattern.is_a? Regexp)
      pattern = Regexp.new(pattern)
    end
    match = pattern.match(self)
    if match
      s = to_s_prim
      replacement = (replacement || block.call(match[0])).to_s
      if match.begin(0) > 0
        s = s.substr(0, match.begin(0)-1) + replacement
      else
        s = replacement
      end
      if match.end(0) < s.length
        s += s.substr(match.end(0), s.length - 1)
      end
      String.new(s)
    else
      self
    end
  end
end
