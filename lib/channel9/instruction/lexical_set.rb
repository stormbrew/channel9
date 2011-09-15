module Channel9
  module Instruction
    # lexical_set depth, name
    # ---
    # Takes the top element off the stack and stores it in the named
    # lexical in the current execution context.
    #
    # Takes one element from the stack:
    #  SP -> element
    # After executing, the element will be removed from the stack.
    class LEXICAL_SET < Base
      attr :lexical_name
      attr :lexical

      def initialize(stream, depth, name)
        super(stream, 1, 0)
        @depth = depth
        @lexical_name = name
        @lexical = stream.lexical(name)
      end

      def arguments
        [@depth, @lexical_name]
      end

      def run(environment)
        val = environment.context.pop
        environment.context.set_lexical(@depth, @lexical, val)
      end
    end
  end
end