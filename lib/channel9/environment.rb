require 'pp'

module Channel9
  # Exits with a status code of 0 regardless of what's passed to it.
  module CleanExitChannel
    def self.channel_send(val, ret)
      exit(0)
    end
  end

  # Exits with the status code passed to it.
  module ExitChannel
    def self.channel_send(val, ret)
      exit(val.to_i)
    end
  end

  # Used as a guard when a sender does not expect to be returned to.
  # Just blows things up.
  module InvalidReturnChannel
    def self.channel_send(val, ret)
      raise "Invalid Return, exiting"
    end
  end

  # Used to output information to stdout. Prints whatever's
  # passed to it.
  module StdoutChannel
    def self.channel_send(val, ret)
      $stdout.puts(val)
      ret.channel_send(val, InvalidReturnChannel)
    end
  end

  class Environment
    attr :context
    attr :special_channel
    attr :debug, true
    attr :running

    def initialize(debug = false)
      @context = nil
      @running = false
      @debug = debug
      @special_channel = {
        :clean_exit => CleanExitChannel,
        :exit => ExitChannel,
        :invalid_return => InvalidReturnChannel,
        :stdout => StdoutChannel
      }
    end

    def run(context)
      @context = context
      if (!@running)
        @running = true
        begin
          while (instruction = @context.next)
            current_context = @context
            sp = @context.stack.length
              pp(:instruction => {:ip => @context.pos - 1, :instruction => instruction.debug_info}) if @debug
              pp(:before => @context.debug_info) if @debug
            instruction.run(self)
              pp(:orig_after_jump => current_context.debug_info) if @debug && current_context != @context
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
        ensure
          @running = false
        end
      end
    end
  end
end