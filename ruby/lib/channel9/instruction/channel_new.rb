module Channel9
  module Instruction
    # channel_new label
    # ---
    # Creates a new channel from the current execution context at the
    # label specified, and pushes it onto the stack.
    #
    # Takes no inputs.
    # After executing, the stack will look like:
    #  SP -> channel
    #
    # The channel created can be used given to the channel_send
    # family of instructions.
    class CHANNEL_NEW < Base
      attr :label

      def initialize(stream, label)
        super(stream, 0, 1)
        @label = label
      end

      def arguments
        [@label]
      end

      def run(environment)
        channel = environment.context.callable(@label)
        
        environment.context.push(channel)
      end
    end
  end
end