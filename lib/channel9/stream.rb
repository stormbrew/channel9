require 'channel9/instruction'

module Channel9
  class Stream # a stream of bytecode with label and local tables
    attr :instructions
    attr :labels
    attr :locals

    def initialize()
      @instructions = []
      @labels = {}
      @locals = {}
      @pos = 0
    end

    def set_label(name)
      @labels[name] = @pos
    end
    def label(name_or_ip)
      @labels[name_or_ip] || name_or_ip
    end

    def local(name)
      @locals[name] ||= @locals.length
    end
    def local_name(id)
      @locals.invert[id]
    end

    def each(from = 0)
      i = label(from)
      while (i < @instructions.length)
        yield @instructions[i]
      end
    end

    # generate methods based on instruction names
    def method_missing(name, *args)
      if (instruction = Instruction.get(name))
        @instructions << instruction.new(self, *args)
        @pos += 1
        return self
      else
        super
      end
    end
  end
end