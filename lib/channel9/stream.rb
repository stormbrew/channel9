require 'channel9/instruction'
require 'channel9/builder'
require 'json'

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
      JSON.pretty_generate(
        "code" => @instructions
      )
    end

    def build
      yield Builder.new(self)
    end
    def self.build(&block)
      Stream.new.build(&block)
    end

    def set_label(name)
      @labels[name] = @pos
    end
    def label(name_or_ip)
      @labels[name_or_ip] || name_or_ip
    end

    def line(*args); end

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

    def <<(instruction)
      @instructions << instruction
      @pos += 1
    end
  end
end