module Channel9
  module Instruction
    # push constant
    # ---
    # Pushes the specified constant onto the stack.
    #
    # Takes no inputs
    # After execution, the stack will have the constant on it:
    #  SP -> constant
    class PUSH < Base
      attr :value

      def initialize(stream, value)
        super(stream, 0, 1)
        @value = value.to_c9
      end

      def arguments
        [@value]
      end

      def run(environment)
        environment.context.push(@value)
      end
    end
  end
end