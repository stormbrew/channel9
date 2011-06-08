module Channel9
  module Instruction
    # is_eq
    # ---
    # Tests whether or not the top two stack entries are
    # equal and pushes a boolean
    # value indicating their equality
    #
    # Takes two values from the stack:
    #  SP -> val1 -> val2
    # And produces their equality
    #  SP -> equal?
    class IS_EQ < Base
      def initialize(stream)
        super(stream, 2, 1)
      end

      def run(environment)
        val1 = environment.context.pop
        val2 = environment.context.pop
        environment.context.push((val1 == val2).to_c9)
      end
    end
  end
end