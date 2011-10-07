module Channel9
  module Instruction
    # debugger type
    # ---
    # Invokes a debug action, as registered with
    # the environment. Does not manipulate the stack.
    #
    # Built in debug actions are:
    # * state: print out the current execution context's state.
    # * start_print_steps: print out execution context's state at
    #   every instruction step until:
    # * stop_print_steps: don't print out execution context's state
    #   at every instruction step anymore.
    class DEBUGGER < Base
      def initialize(stream, type)
        super(stream)
        @type = type
      end

      def arguments
        [@type]
      end
      def run(env)
        env.do_debug_handler(@type)
      end
    end
  end
end