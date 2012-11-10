module Channel9
  module Instruction
    # swap
    # ---
    # Switches the top two elements of the stack.
    #
    # Takes two input values
    #  SP -> input1 -> input2
    # Puts them back on the stack in reverse order.
    #  SP -> input2 -> input1
    class SWAP < Base
      def initialize(stream)
        super(stream, 2, 2)
      end
      def run(environment)
        input1 = environment.context.pop
        input2 = environment.context.pop
        environment.context.push(input1)
        environment.context.push(input2)
      end
    end
  end
end