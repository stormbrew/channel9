class String
  def initialize(str)
    @str = str.to_s_prim
  end

  def split(by)
    @str.split(by.to_s_prim).to_a.collect {|i| i.to_s }
  end

  def [](of, len = nil)
    if (of.class == Range)
      first = of.begin
      last = of.end
      last -= 1 if (of.exclude_end?)
      @str.substr(first, last).to_s
    elsif (of.class == Fixnum)
      if (len.nil?)
        @str.substr(of, of).to_chr
      else
        @str.substr(of, of+len).to_s
      end
    else
      raise NotImplementedError, "Unknown index type for String#[]"
    end
  end

  def hash
    @str.hash
  end

  def length
    @str.length
  end
  def empty?
    @str.length == 0
  end
  def size
    length
  end

  def to_sym
    @str
  end
  def to_primitive_str
    @str
  end
  def to_s_prim
    @str
  end
  def to_s
    self
  end
  def to_str
    self
  end
  def to_i
    @str.to_num_primitive
  end

  def ljust(i, pad)
    self # TODO: Make not stub
  end
  def chomp
    self # TODO: Make not a stub
  end

  def +(other)
    String.new(@str + other.to_s_prim)
  end
  def <<(other)
    @str = @str + other.to_s_prim
    self
  end

  def ==(other)
    @str == other.to_s_prim
  end
  def eql?(other)
    @str.eql?(other.to_s_prim)
  end
  def ===(other)
    @str == other.to_s_prim
  end
  def <=>(other)
    @str.compare(other.to_s_prim)
  end
end
