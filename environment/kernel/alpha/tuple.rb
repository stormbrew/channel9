module Channel9
  class Tuple
    alias_method :[], :at
    alias_method :[]=, :put

    def to_a
      Array.new(self)
    end
  end
end