module Channel9
  # Dynamic bytecode stream builder.
  class Builder
    attr :stream
    attr :label_counter

    def initialize(stream)
      @stream = stream
      @label_counter = 0
    end

    def make_label(prefix)
      "#{prefix}.#{@label_counter += 1}"
    end

    def set_label(name)
      @stream.set_label(name)
    end

    def line(filename, line, pos = nil)
      @stream.line(filename, line, pos = nil)
    end

    # generate methods based on instruction names
    def method_missing(name, *args)
      if (instruction = Instruction.get(name))
        @stream << instruction.new(@stream, *args)
        return self
      else
        super
      end
    end
  end
end