class MatchData
  def initialize(regex, str, matches)
    @regex = regex
    @str = str
    @matches = matches
  end

  def begin(idx)
    match = @matches.at(idx.to_i)
    if (match && match.at(0) >= 0 && match.at(1) >= 0)
      return match.at(0)
    end
    nil
  end
  def end(idx)
    match = @matches.at(idx.to_i)
    if (match && match.at(0) >= 0 && match.at(1) >= 0)
      return match.at(1)
    end
    nil
  end

  def offset(idx)
    match = @matches.at(idx.to_i)
    if (match && match.at(0) >= 0 && match.at(1) >= 0)
      return match.to_a
    end
    nil
  end

  def pre_match
    String.new(@str.substr(0, @matches.at(0).at(0)))
  end
  def post_match
    String.new(@str.substr(@matches.at(0).at(1), -1))
  end
  def to_s
    String.new(@str.substr(@matches.at(0).at(0), @matches.at(0).at(1)-1))
  end

  def length
    return @matches.length
  end

  def to_a
    @matches.map do |match|
      if (match && match.at(0) >= 0 && match.at(1) >= 0)
        String.new(@str.substr(match.at(0), match.at(1)-1))
      else
        nil
      end
    end
  end

  def [](idx, len = nil)
    match = @matches.at(idx.to_i)
    if (match && match.at(0) >= 0 && match.at(1) >= 0)
      return String.new(@str.substr(match.at(0), match.at(1)-1))
    end
    return nil
  end

  def values_at(*ids)
    ids.map do |id|
      self[id]
    end
  end
end

class Regexp
  def initialize(s)
    @regex_str = s
    err, errstr, @matcher = $__c9_regexp.matcher(s.to_s_prim)
    if errstr
      raise errstr # TODO: raise the right error type
    end
  end

  def c9_regex_str
    @regex_str
  end
  def eql?(o)
    o.is_a?(Regexp) && @regex_str == o.c9_regex_str
  end
  def ==(o)
    eql?(o)
  end
  def ===(o)
    !self.match(o).nil?
  end
  def =~(o)
    match = match(o)
    if match
      return match.begin(0)
    else
      return nil
    end
  end

  def match(s)
    s = s.to_s_prim
    err, errstr, results = @matcher.match(s)

    if errstr
      raise errstr # TODO: raise the right error type.
    end

    if results.nil?
      return nil
    end
    MatchData.new(self, s, results)
  end

  def c9_gsub(s, replacement, &block)
    replacements = []
    s = s.to_s_prim
    replacement = replacement.to_s_prim if replacement
    pos = 0
    while true
      err, errstr, results = @matcher.match(s, pos)
      if errstr
        raise errstr # TODO: raise the right error type.
      end

      if results.nil?
        break
      end
      with = replacement || block.call(String.new(s.substr(results.at(0).at(0), results.at(0).at(1)-1))).to_s_prim
      replacements.push([results.at(0), with])
      pos = results.at(0).at(1)
    end

    if replacements.length > 0
      res = "".to_s_prim
      pos = 0
      replacements.each do |replacement|
        range, with = replacement
        res = res + s.substr(pos, range.at(0)-1) + with
        pos = range.at(1)
      end
      res = res + s.substr(pos, s.length-1) if pos < s.length
      return String.new(res)
    else
      return s
    end
  end
end
