module Channel9
  module Instruction
    # message_splat
    # ---
    # Takes a message and a tuple off the stack, appends the
    # contents of the tuple to the message, and returns the
    # resulting message.
    #
    # Takes message and tuple:
    #  SP -> tuple -> message
    # Leaves the message on the stack:
    #  SP -> message
    #
    class MESSAGE_SPLAT < Base
      def initialize(stream)
        super(stream, 2, 1)
      end

      def run(env)
        tuple = env.context.pop
        msg = env.context.pop
        env.context.push(msg.splat(tuple.real_ary))
      end
    end
  end
end