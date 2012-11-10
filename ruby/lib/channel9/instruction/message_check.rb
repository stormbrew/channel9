module Channel9
  module Instruction
    # message_check
    # ---
    # Checks whether the object at the top of the stack is a message
    # and leaves it on if so, otherwise pushes false.
    #
    # Takes a object as input
    #  SP -> obj
    # Leaves it on if it is a message, pushes false otherwise.
    #  SP -> message_or_false
    class MESSAGE_CHECK < Base
      def initialize(stream)
        super(stream, 1, 1)
      end
    end
  end
end