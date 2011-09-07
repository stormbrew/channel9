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

    def initialize(environment, stream)
      @environment = environment
      @instruction_stream = stream
    end

    def truthy?
      true
    end

    def to_c9
      self
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

    def debug_info
      n = @pos - 7
      j, k = 0, 0
      {
        :is => @instruction_stream.to_s, 
        :ip => @pos-1,
        :context => @instruction_stream.instructions[@pos-6, 10].collect {|i| "#{n += 1}: #{i}" },
        :line => line_info,
        :locals => Hash[@local_variables.collect {|l| l.collect {|v| j += 1; [@instruction_stream.local_name(j-1), v.to_s] }}], 
        :frame => Hash[@frame_variables.collect {|v| k += 1; [@instruction_stream.framevar_name(k-1), v.to_s] }], 
        :stack => @stack.collect {|x| x.to_s } 
      }
    end
  end
end