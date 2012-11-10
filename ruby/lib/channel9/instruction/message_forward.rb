module Channel9
  module Instruction
    # message_forward name
    # ---
    # Changes the name of the message and pushes the old name as
    # the first argument. 
    #
    # Takes the message off the stack
    #  SP -> message
    # Pushes the new message
    #  SP -> new_message
    class MESSAGE_FORWARD < Base
      def initialize(stream, name)
        super(stream, 1, 1)
        @name = name
      end

      def arguments
        [@name]
      end
    end
  end
end