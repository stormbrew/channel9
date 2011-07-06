require 'channel9/instruction'
require 'channel9/builder'
require 'json'

module Channel9
  class Stream # a stream of bytecode with label and local tables
    attr :instructions
    attr :labels
    attr :locals
    attr :line_info
    attr :framevars

    def initialize()
      @instructions = []
      @labels = {}
      @locals = {}
      @line_info = []
      @framevars = {}
      @pos = 0
    end

    def from_json(json)
      o = JSON.parse(json)
      builder = Builder.new(self)
      code = o["code"]
      code.each do |instruction|
        builder.send(*instruction)
      end
      self
    end
    def self.from_json(json)
      new.from_json(json)
    end

    def to_json(*a)
      labels = @labels.invert
      instructions = []
      i = 0
      while (i < @instructions.length)
        instructions << ["set_label", labels[i]] if (labels[i])
        instructions << ["line", *line_info[i]] if (line_info[i])
        instructions << @instructions[i]
        i += 1
      end

      JSON.pretty_generate(
        "code" => instructions
      )
    end

    def build
      yield Builder.new(self)
    end
    def self.build(&block)
      Stream.new.build(&block)
    end

    def set_label(name)
      add_label(name)
      @labels[name] = @pos
    end
    def label(name_or_ip)
      @labels[name_or_ip] || name_or_ip
    end

    def line(file, line, fpos = 0, extra = "")
      while (@line_info.length <= @pos)
        @line_info << nil
      end
      add_line_info(file, line, fpos, extra)
      @line_info[@pos] = file, line, fpos
    end

    def local(name)
      @locals[name] ||= @locals.length
    end
    def local_name(id)
      @locals.invert[id]
    end
    def framevar(name)
      @framevars[name] ||= @framevars.length
    end
    def framevar_name(id)
      @framevars.invert[id]
    end

    def each(from = 0)
      i = label(from)
      while (i < @instructions.length)
        yield @instructions[i]
      end
    end

    def <<(instruction)
      @instructions << instruction
      add_instruction(instruction.instruction_name, instruction.arguments)
      @pos += 1
    end
  end
end