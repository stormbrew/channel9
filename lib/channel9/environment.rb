require 'pp'

module Channel9
  # Exits with a status code of 0 regardless of what's passed to it.
  module CleanExitChannel
    def self.channel_send(env, val, ret)
      exit(0)
    end
    def self.truthy?; true; end
  end

  # Exits with the status code passed to it.
  module ExitChannel
    def self.channel_send(env, val, ret)
      exit(val.to_i)
    end
    def self.truthy?; true; end
  end

  # Used as a guard when a sender does not expect to be returned to.
  # Just blows things up.
  module InvalidReturnChannel
    def self.channel_send(env, val, ret)
      raise "Invalid Return, exiting"
    end
    def self.truthy?; true; end
  end

  # Used to output information to stdout. Prints whatever's
  # passed to it.
  module StdoutChannel
    def self.channel_send(env, val, ret)
      $stdout.puts(val)
      ret.channel_send(env, val, InvalidReturnChannel)
    end
    def self.truthy?; true; end
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

    # stores the context for the duration of a block and then
    # restores it when done. Used for instructions that need
    # to call back into the environment. (eg. string_new).
    # in most cases, you will probably need to indicate when to
    # exit the sub-state by throwing :end_save.
    def save_context
      catch (:end_save) do
        begin
          prev_context = @context
          prev_running = @running
          @context = nil
          @running = false
          yield
        ensure
          @running = true
          @context = prev_context
        end
      end
    end

    def run_debug(context)
      @context = context
      if (!@running)
        @running = true
        begin
          while (instruction = @context.next)
            current_context = @context
            sp = @context.stack.length
            if (@debug == :detail)
              pp(:instruction => {:ip => @context.pos - 1, :instruction => instruction.debug_info})
              pp(:before => @context.debug_info)
            end
            instruction.run(self)
            if (@debug == :detail)
              pp(:orig_after_jump => current_context.debug_info) if current_context != @context
              pp(:after => @context.debug_info)
              puts("--------------")
            end
            stack_should = (sp - instruction.stack_input + instruction.stack_output)
            if (current_context == @context &&
                current_context.stack.length != stack_should)
              raise "Stack error: Expected stack depth to be #{stack_should}, was actually #{current_context.stack.length}"
            end
          end
          if (@debug == :detail)
            pp(
              :final_state => @context.debug_info
            )
          end
        ensure
          @running = false
        end
      end
    end

    def run(context)
      return run_debug(context) if @debug
      @context = context
      if (!@running)
        @running = true
        begin
          while (instruction = @context.next)
            current_context = @context
            sp = @context.stack.length
            instruction.run(self)
          end
        ensure
          @running = false
        end
      end
    end
  end
end