module Channel9
  module Instruction
    # channel_call
    # ---
    # A less primitive (and limited) version of channel_send, this instruction
    # automatically constructs a return channel from the current execution
    # context (resume on next instruction). The following bytecode sequences
    # can be seen as being roughly equivalent:
    #
    #   channel_special :stdout
    #   push 1
    #   channel_call
    #
    #   channel_special :stdout
    #   new_channel :resume
    #   push 1
    #   channel_send
    #   resume:
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
    #  SP -> caller_context -> value
    # which allows for the target channel to simply push a new value and return location
    # on to its stack in order to send back.
    class CHANNEL_CALL < Base
      def initialize(stream)
        super(stream, 2)
      end

      # assumes the channel return is the next instruction (aka, the state of the current context)..
      def run(environment)
        value = environment.context.pop
        channel = environment.context.pop

        channel.channel_send(environment, value, environment.context.dup)
      end
    end
  end
end