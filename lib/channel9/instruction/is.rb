module Channel9
  module Instruction
    # is const
    # ---
    # Tests whether or not the top stack entry is the same
    # as the constant given.
    #
    # Takes one value from the stack:
    #  SP -> val
    # And produces its equality with the given constant
    #  SP -> equal?
    class IS < Base
      def initialize(stream, const)
        super(stream, 1, 1)
        @const = const.to_c9
      end

      def arguments
        [@const]
      end

      def run(environment)
        val = environment.context.pop
        environment.context.push((val == @const).to_c9)
      end
    end
  end
end