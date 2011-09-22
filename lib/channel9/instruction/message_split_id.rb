module Channel9
  module Instruction
    # message_split_id
    # ---
    # Takes the message on top of the stack and pushes the message
    # id and protocol id (seperated) back onto the stack along with
    # the original message.
    #
    # Takes a message as input
    #  SP -> message
    # Leaves the flag and message on the stack
    #  SP -> protocol_id -> message_id -> message
    class MESSAGE_SPLIT_ID < Base
      def initialize(stream)
        super(stream, 1, 3)
      end
    end
  end
end