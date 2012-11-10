module Channel9
  module Instruction
    # message_is_proto protocol
    # ---
    # Takes the message on top of the stack, determines if the message
    # is for the protocol given, and then pushes the message and a boolean
    # flag back on the stack.
    #
    # Takes a message as input
    #  SP -> message
    # Leaves the flag and message on the stack
    #  SP -> flag -> message
    class MESSAGE_IS_PROTO < Base
      def initialize(stream, msgname)
        super(stream, 1, 2)
        @protocol
      end

      def arguments
        [@protocol]
      end
    end
  end
end