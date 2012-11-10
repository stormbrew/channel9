module Channel9
  module Instruction
    # is_not const
    # ---
    # Tests whether or not the top stack entry is different
    # from the constant given.
    #
    # Takes one value from the stack:
    #  SP -> val
    # And produces its inequality with the given constant
    #  SP -> not_equal?
    class IS_NOT < Base
      def initialize(stream, const)
        super(stream, 1, 1)
        @const = const
      end

      def arguments
        [@const]
      end

      def run(environment)
        val = environment.context.pop
        environment.context.push(val != @const)
      end
    end
  end
end