module Channel9
  module Instruction
    # message_is msgname
    # ---
    # Takes the message on top of the stack, determines if the message
    # id matches the id given, and then pushes the message and a boolean
    # flag back on the stack.
    #
    # Takes a message as input
    #  SP -> message
    # Leaves the flag and message on the stack
    #  SP -> flag -> message
    class MESSAGE_IS < Base
      def initialize(stream, msgname)
        super(stream, 1, 2)
        @msgname = msgname
      end

      def arguments
        [@msgname]
      end
    end
  end
end