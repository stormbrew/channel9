module Channel9
  module Instruction
    # message_name
    # ---
    # Takes the message on top of the stack, retrieves its name,
    # and pushes both the name and the message back onto the stack.
    #
    # Takes a message as input
    #  SP -> message
    # Leaves the name and message on the stack:
    #  SP -> name -> message
    class MESSAGE_NAME < Base
      def initialize(stream)
        super(stream, 1, 2)
      end

      def run(env)
        message = env.context.pop
        env.context.push(message)
        env.context.push(message.name)
      end
    end
  end
end