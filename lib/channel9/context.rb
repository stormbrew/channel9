module Channel9
  class Context
    attr :instruction_stream
    attr :pos
    attr :local_variables
    attr :stack

    def initialize(stream)
      @instruction_stream = stream
      @pos = 0
      @local_variables = {}
      @stack = []
    end

    def initialize_copy(other)
      super
      @stack = @stack.dup
    end

    def set_pos(pos)
      @pos = @instruction_stream.label(pos)
      return self
    end

    def reset_stack
      @stack = []
      return self
    end

    def channel_send(environment, val, ret)
      copy = self.dup
      copy.push val
      copy.push ret
      environment.set_context(copy)
      return self
    end

    def start(stack = [])
      @stack = stack
      return self
    end
    
    def push(val)
      @stack.push(val)
    end
    def pop
      @stack.pop
    end

    def get_local(name)
      @local_variables[@instruction_stream.local(name)]
    end
    def set_local(name, val)
      @local_variables[@instruction_stream.local(name)] = val
    end

    def next
      n = @instruction_stream.instructions[@pos]
      @pos += 1 if n
      return n
    end

    def debug_info
      {
        :is => @instruction_stream.to_s, 
        :ip => @pos, 
        :locals => Hash[@local_variables.collect {|k,v| [@instruction_stream.local_name(k), v.to_s] }], 
        :stack => @stack.collect {|x| x.to_s } 
      }
    end
  end
end