module Channel9
  module Instruction
    # message_sys_prefix count
    # ---
    # Adds count items from the stack to the front of a message and
    # pushes the new message.
    # 
    # Takes the arguments and message at the top of the stack
    #  SP -> argN ... -> arg1 -> message
    # Pushes the new message
    #  SP -> new_message
    class MESSAGE_SYS_PREFIX < Base
      def initialize(stream, count)
        super(stream, 1, 1 + count)
        @count = count
      end

      def arguments
        [@count]
      end
    end
  end
end