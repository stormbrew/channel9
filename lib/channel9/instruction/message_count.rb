module Channel9
  module Instruction
    # message_count
    # ---
    # Takes the message on top of the stack, retrieves the number of
    # normal positional arguments it contains, and pushes both the count
    # and the message back onto the stack.
    #
    # Takes a message as input
    #  SP -> message
    # Leaves the count and message on the stack:
    #  SP -> count -> message
    class MESSAGE_COUNT < Base
      def initialize(stream)
        super(stream, 1, 2)
      end

      def run(env)
        message = env.context.pop
        env.context.push(message)
        env.context.push(message.positional.length.to_c9)
      end
    end
  end
end