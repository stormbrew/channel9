require 'pp'

module Channel9
  class Environment
    attr :context
    attr :debug, true

    def initialize(initial_context, debug = true)
      @context = initial_context
      @debug = debug
    end

    def set_context(context)
      @context = context
    end

    def run
      while (instruction = @context.next)
        current_context = @context
        sp = @context.stack.length
          pp(:instruction => {:ip => @context.pos - 1, :instruction => instruction.debug_info}) if @debug
          pp(:before => @context.debug_info) if @debug
        instruction.run(self)
          pp(:after => @context.debug_info) if @debug
        puts("--------------") if @debug
        stack_should = (sp - instruction.stack_input + instruction.stack_output)
        if (current_context.stack.length != stack_should)
          raise "Stack error: Expected stack depth to be #{stack_should}, was actually #{current_context.stack.length}"
        end
      end
      if (@debug)
        require 'pp'
        pp(
          :final_state => @context.debug_info
        )
      end
    end
  end
end