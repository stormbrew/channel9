require 'pp'

module Channel9
  # Exits with a status code of 0 regardless of what's passed to it.
  module CleanExitChannel
    def self.channel_send(env, val, ret)
      exit(0)
    end
    def self.truthy?; true; end
    def self.to_c9; self; end
  end

  # Exits with the status code passed to it.
  module ExitChannel
    def self.channel_send(env, val, ret)
      if (val.kind_of? Primitive::Message)
        exit(val.positional[0])
      else
        exit(val.real_num)
      end
    end
    def self.truthy?; true; end
    def self.to_c9; self; end
  end

  # Used as a guard when a sender does not expect to be returned to.
  # Just blows things up.
  module InvalidReturnChannel
    def self.channel_send(env, val, ret)
      raise "Invalid Return, exiting"
    end
    def self.truthy?; true; end
    def self.to_c9; self; end
  end

  # Used to output information to stdout. Prints whatever's
  # passed to it.
  module StdoutChannel
    def self.channel_send(env, val, ret)
      $stdout.puts(val)
      ret.channel_send(env, val, InvalidReturnChannel)
    end
    def self.truthy?; true; end
    def self.to_c9; self; end
  end

  class Environment
    attr :debug, true
    attr :running

    def initialize(debug = false)
      @context = nil
      @running = false
      @debug = debug
      @saved_debug = debug
      
      set_special_channel(:clean_exit, CleanExitChannel);
      set_special_channel(:exit, ExitChannel);
      set_special_channel(:invalid_return, InvalidReturnChannel);
      set_special_channel(:stdout, StdoutChannel);

      @debug_handlers = {}
      register_debug_handler(:print) do |env, ctx|
        pp(:debug_print => {
          :instruction => {:ip => ctx.pos - 1, :instruction => ctx.next.debug_info},
          :context => ctx.debug_info}
        )
      end
      register_debug_handler(:start_print_steps) do
        @saved_debug, @debug = @debug, :detail
      end
      register_debug_handler(:stop_print_steps) do
        @debug = @saved_debug
      end
      @error_handler = proc {|err, ctx|
        puts "Unhandled VM Error in #{self}"
        pp ctx.debug_info
        raise err
      }
    end

    def set_error_handler(&handler)
      @error_handler = handler
    end

    def register_debug_handler(name, &exec)
      @debug_handlers[name.to_sym] = exec
    end
    def do_debug_handler(name)
      ctx = @context
      save_context do
        @debug_handlers[name.to_sym].call(self, ctx)
      end
    end

    # stores the context for the duration of a block and then
    # restores it when done. Used for instructions that need
    # to call back into the environment. (eg. string_new).
    # in most cases, you will probably need to indicate when to
    # exit the sub-state by throwing :end_save.
    def save_context
      catch (:end_save) do
        begin
          prev_context = current_context
          set_current_context(nil)
          yield
        ensure
          set_current_context(prev_context)
        end
      end
    end

    def no_debug
      begin
        @debug, debug = false, @debug
        yield
      ensure
        @debug = debug
      end
    end

    def for_debug(level = true)
      begin
        @debug, debug = false, @debug
        case level
        when true
          yield if debug
        when :detail
          yield if debug == :detail
        end
      ensure
        @debug = debug
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
            for_debug(:detail) do
              pp(:instruction => {:ip => @context.pos - 1, :instruction => instruction.debug_info})
              pp(:before => @context.debug_info)
            end
            instruction.run(self)
            for_debug(:detail) do
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
          for_debug(:detail) do
            pp(
              :final_state => @context.debug_info
            )
          end
        rescue => e
          @error_handler.call(e, @context)
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
        rescue => e
          @error_handler.call(e, @context)
        ensure
          @running = false
        end
      end
    end
  end
end