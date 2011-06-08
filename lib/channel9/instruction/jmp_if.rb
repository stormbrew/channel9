module Channel9
  module Instruction
    # jmp_if label
    # ---
    # Jumps to label if the item on the top of the stack is truthy.
    #
    # Takes one item from the stack.
    #  SP -> value
    # And leaves nothing on the stack.
    class JMP_IF < Base
      attr :to

      def initialize(stream, to)
        super(stream, 1, 0)
        @to = to
      end

      def run(environment)
        value = environment.context.pop
        environment.context.set_pos(@to) if (value.truthy?)
      end
    end
  end
end