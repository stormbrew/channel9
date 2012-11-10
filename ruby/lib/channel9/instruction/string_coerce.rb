module Channel9
  module Instruction
    # string_coerce message_name
    # ---
    # Takes an object off the stack, pushes it and nil back
    # onto the stack if it was already a string, otherwise
    # sends a unary message to the object of message_name
    # which must return a string object.
    #
    # Takes one item from the top of the stack:
    #  SP -> obj
    # Pushes a new string onto the stack in their place:
    #  SP -> ret -> string
    class STRING_COERCE < Base
      def initialize(stream, coerce)
        super(stream, 1, 2)
        @coerce = coerce
      end

      def arguments
        [@coerce]
      end
    end
  end
end