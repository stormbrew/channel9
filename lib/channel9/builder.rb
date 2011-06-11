module Channel9
  # Dynamic bytecode stream builder.
  class Builder
    attr :stream

    def initialize(stream)
      @stream = stream
    end

    def set_label(name)
      @stream.set_label(name)
    end

    def line(*args); 
      @stream.line(*args)
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