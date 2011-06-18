module Channel9
  module Instruction
    # local_set name
    # ---
    # Takes the top element off the stack and stores it in the named
    # local in the current execution context.
    #
    # Takes one element from the stack:
    #  SP -> element
    # After executing, the element will be removed from the stack.
    class LOCAL_SET < Base
      attr :local_name
      attr :local

      def initialize(stream, name)
        super(stream, 1, 0)
        @local_name = name
        @local = stream.local(name)
      end

      def arguments
        [@local_name]
      end

      def run(environment)
        val = environment.context.pop
        environment.context.set_local(@local, val)
      end
    end
  end
end