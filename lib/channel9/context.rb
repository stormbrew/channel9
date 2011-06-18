module Channel9
  class CallbackChannel
    def initialize(&cb)
      @callback = cb
    end
    def to_c9; self; end
    def channel_send(env, val, ret)
      @callback.call(env, val, ret)
    end
  end

  class Context
    attr :environment
    attr :instruction_stream
    attr :pos
    attr :local_variables
    attr :frame_variables
    attr :stack

    def initialize(environment, stream, pos = 0, locals = nil, framevars = nil)
      @environment = environment
      @instruction_stream = stream
      @pos = pos
      @local_variables = locals ? locals : Array.new(@instruction_stream.locals.length)
      @frame_variables = framevars ? framevars.dup : Array.new(@instruction_stream.framevars.length)
      @stack = []
    end

    def truthy?
      true
    end

    def initialize_copy(other)
      super
      @stack = @stack.dup
    end

    def callable(pos)
      CallableContext.new(@environment, @instruction_stream, 
        @instruction_stream.label(pos), @local_variables, @frame_variables)
    end

    def set_pos(pos)
      @pos = @instruction_stream.label(pos)
      return self
    end

    def reset_stack
      @stack = []
      return self
    end

    def channel_send(env, val = nil, ret = CleanExitChannel)
      push val
      push ret
      @environment.run(self)
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

    def get_local(id)
      @local_variables[id]
    end
    def set_local(id, val)
      @local_variables[id] = val
    end
    def clean_scope
      @local_variables = Array.new(@instruction_stream.locals.length)
    end

    def get_framevar(id)
      @frame_variables[id]
    end
    def set_framevar(id, val)
      @frame_variables[id] = val
    end

    def next
      n = @instruction_stream.instructions[@pos]
      @pos += 1 if n
      return n
    end

    def debug_info
      j, k = 0, 0
      {
        :is => @instruction_stream.to_s, 
        :ip => @pos, 
        :locals => Hash[@local_variables.collect {|v| j += 1; [@instruction_stream.local_name(j-1), v.to_s] }], 
        :frame => Hash[@frame_variables.collect {|v| k += 1; [@instruction_stream.framevar_name(k-1), v.to_s] }], 
        :stack => @stack.collect {|x| x.to_s } 
      }
    end
  end

  class CallableContext
    def initialize(env, stream, pos, locals, framevars)
      @env = env
      @stream = stream
      @pos = pos
      @locals = locals
      @framevars = framevars.dup
    end

    def channel_send(cenv, val, ret)
      Context.new(@env, @stream, @pos, @locals, @framevars).channel_send(cenv, val, ret)
    end
  end
end