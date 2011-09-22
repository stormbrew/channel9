module Channel9
  module Instruction
    # message_id
    # ---
    # Takes the message on top of the stack and pushes the message
    # id back onto the stack along with the original message.
    #
    # Takes a message as input
    #  SP -> message
    # Leaves the flag and message on the stack
    #  SP -> id -> message
    class MESSAGE_ID < Base
      def initialize(stream)
        super(stream, 1, 2)
      end
    end
  end
end