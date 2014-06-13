class String
  include Comparable

  def %(vals)
    sprintf(self, *vals)
  end

  def *(times)
    s = ""
    while times > 0
      s += self
      times -= 1
    end
    s
  end

  def match(pattern, pos)
    if (!pattern.is_a? Regexp)
      pattern = Regexp.new(pattern)
    end
    pattern.match(self, pos)
  end

  def lines(by=$/)
    res = split(by || '\n')
    if block_given?
      res.each do |s|
        yield s
      end
    else
      res
    end
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

  def end_with?(str)
    /#{Regexp.escape(str)}$/.match(self) ? true : false
  end
  def start_with?(str)
    str = str.to_s_prim
    return @str.length > str.length && @str.substr(0,str.length-1) == str
  end

  def index(search, offset = 0)
    if !search.is_a? Regexp
      search = /#{Regexp.escape(search.to_s)}/
    end
    return search.c9_index(@str, offset)
  end

  def upcase
    # Really simple non-unicode implementation
    gsub(/[a-z]/) do |c|
      "%c" % (c[0] & (0xff ^ 32))
    end
  end
  def downcase
    gsub(/[A-Z]/) do |c|
      "%c" % (c[0] | 32)
    end
  end

  def unpack(pattern)
    # For now this will only implement C* because that's what Parser wants.
    # TODO: Implement properly, possibly in C++.
    if pattern == "C*"
      prim = @str
      len = prim.length
      i = 0
      res = []
      while i < len
        res << prim.substr(i,i).to_chr
        i += 1
      end
      res
    else
      raise "Unsupported unpack pattern '#{pattern}'"
    end
  end

  def each_char
    prim = @str
    len = prim.length
    i = 0
    if block_given?
      while i < len
        yield prim.substr(i,i)
        i += 1
      end
    else
      res = []
      while i < len
        res << String.new(prim.substr(i,i))
        i+=1
      end
      res
    end
  end

  def inspect
    "\"#{gsub(/"/, '\\"')}\""
  end
end
