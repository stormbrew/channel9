module Channel9
  module Instruction
    # pop
    # ---
    # Removes the top element from the stack.
    #
    # Takes one arbitrary input:
    #  SP -> value
    # After executing, the stack will no longer have that element on it.
    class POP < Base
      def initialize(stream)
        super(stream, 1, 0)
      end
      def run(environment)
        environment.context.pop
      end
    end
  end
end