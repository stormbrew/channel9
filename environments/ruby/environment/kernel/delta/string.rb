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


  def split(by=$;, limit = nil)
    return [] if empty?

    by ||= ' '
    splits = case by
    when ' '
      /\s+/.c9_split(@str, true, limit)
    when String
      if !limit || limit < 0
        @str.split(by.to_s_prim).collect {|i| i.to_s }
      else
        /#{by}/.c9_split(@str, false, limit)
      end
    when Regexp
      by.c9_split(@str, false, limit)
    else
      raise "Unknown argument type passed to split (TODO: Should coerce to string?)."
    end
    if !limit
      # If the limit isn't passed, cut off trailing empty items.
      while splits.last && splits.last.length == 0
        splits.pop
      end
    end
    splits
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
  def chomp
    sub(%r{\n$}, '')
  end
  def chomp!
    @str = chomp.to_s_prim
  end
end
