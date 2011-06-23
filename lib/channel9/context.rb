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
    attr :parent_locals
    attr :frame_variables
    attr :stack
    attr :caller

    def initialize(environment, stream, pos = 0, locals = nil, framevars = nil, caller = nil)
      @environment = environment
      @instruction_stream = stream
      @pos = pos
      @local_variables = locals ? locals.dup : [Array.new(@instruction_stream.locals.length)]
      @frame_variables = framevars ? framevars.dup : Array.new(@instruction_stream.framevars.length)
      @stack = []
      @caller = caller
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

    def line_info
      # scan back through the instruction stream's line info for the nearest line
      # declaration.
      cur_pos = @pos
      lines = @instruction_stream.line_info
      while (cur_pos >= 0)
        if (!lines[cur_pos].nil?)
          return lines[cur_pos]
        end
        cur_pos -= 1
      end
      return nil
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

    def get_local(depth, id)
      if (depth < @local_variables.length)
        @local_variables[depth][id]
      else
        Primitive::Undef
      end
    end

    def set_local(depth, id, val)
      if (depth < @local_variables.length)
        @local_variables[depth][id] = val
      else
        Primitive::Undef
      end
    end
    def clean_scope
      @local_variables = [Array.new(@instruction_stream.locals.length)]
    end
    def linked_scope
      @local_variables.unshift(Array.new(@instruction_stream.locals.length))
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
        :line => line_info,
        :locals => Hash[@local_variables.collect {|l| l.collect {|v| j += 1; [@instruction_stream.local_name(j-1), v.to_s] }}], 
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
      Context.new(@env, @stream, @pos, @locals, @framevars, @env.context).channel_send(cenv, val, ret)
    end
  end
end