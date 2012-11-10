module Channel9
  module Instruction
    # message_add count
    # ---
    # Takes a message and N items off the stack, appends the
    # items to the message, and returns the
    # resulting message.
    #
    # Takes message and arguments:
    #  SP -> arg1 -> arg2 -> ... argN -> message
    # Leaves the message on the stack:
    #  SP -> message
    #
    class MESSAGE_ADD < Base
      def initialize(stream, count)
        super(stream, 1 + count, 1)
        @count = count
      end

      def arguments
        [@count]
      end

      def run(env)
        args = []
        while (i < @count)
          args << env.context.pop
          i += 1
        end
        msg = env.context.pop

        env.context.push(msg.splat(args))
      end
    end
  end
end