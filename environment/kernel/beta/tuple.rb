module Channel9
  class Tuple
    alias [] at
    alias []= put

    def to_a
      Array.new(self)
    end
  end
end