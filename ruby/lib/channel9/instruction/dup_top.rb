module Channel9
  module Instruction
    # dup_top
    # ---
    # Pushes the top of the stack twice.
    #
    # Takes one input value
    #  SP -> input
    # Leaves it on the stack twice
    #  SP -> input -> input
    class DUP_TOP < Base
      def initialize(stream)
        super(stream, 1, 2)
      end
      def run(environment)
        input = environment.context.pop
        environment.context.push(input)
        environment.context.push(input)
      end
    end
  end
end