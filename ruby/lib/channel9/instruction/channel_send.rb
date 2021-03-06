module Channel9
  module Instruction
    # channel_send
    # ---
    # The most primitive and general instruction for transfering execution
    # to a channel.
    #
    # Takes three inputs:
    #  SP -> value -> return_channel -> target_channel
    # Returns no values, execution continues from target channel.
    #
    # Moves the current execution context to the position of target_channel,
    # pushing return_channel and value onto its stack and continuing execution
    # from that point.
    #
    # The stack in the target channel will, after execution resumes, look like this:
    #  SP -> return_channel -> value
    # which allows for the target channel to simply push a new value and return location
    # on to its stack in order to send back.
    class CHANNEL_SEND < Base
      def initialize(stream)
        # after value is only checked if the channel was special and immediately pushed
        # back to the original context, in which case it will have the return info
        # on the stack.
        super(stream, 3, 2)
      end

      def run(environment)
        value = environment.context.pop
        ret = environment.context.pop
        channel = environment.context.pop

        channel.channel_send(environment, value, ret)
      end
    end
  end
end