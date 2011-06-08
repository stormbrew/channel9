module Channel9
  module Instruction
    # channel_ret
    # ---
    # A less primitive (and limited) version of channel_send, this instruction
    # automatically passes an invalid return channel to as the return channel. 
    # The invalid return channel will raise an error if it's called, it is
    # only intended to guard against an unintended return.
    # The following bytecode sequences can be seen as being roughly equivalent:
    #
    #   get_local :output
    #   push 1
    #   channel_ret
    #
    #   get_local :output
    #   channel_special :invalid_return
    #   push 1
    #   channel_send
    #
    # Takes two inputs:
    #  SP -> value -> target_channel
    # Returns no values, execution continues from target channel.
    #
    # Moves the current execution context to the position of target_channel,
    # pushing the current execution context and value onto its stack and 
    # continuing execution from that point.
    #
    # The stack in the target channel will, after execution resumes, look like this:
    #  SP -> invalid_return_channel -> value
    # The caller should not call the channel given.
    class CHANNEL_RET < Base
      def initialize(stream)
        super(stream, 2)
      end

      def run(environment)
        value = environment.context.pop
        channel = environment.context.pop

        channel.channel_send(environment, value, InvalidReturnChannel)
      end
    end
  end
end