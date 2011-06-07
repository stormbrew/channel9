module Channel9
  module Instruction
    # channel_special name
    # ---
    # Pushes the named 'special channel' onto the stack.
    #
    # Takes no inputs.
    # After executing, the stack will look like:
    #  SP -> special_channel
    #
    # A special channel is a channel that has been registered in the
    # environment as something that should be reachable from any point
    # within that environment. This can be something like the exception
    # raising channel, the exit channel (for exiting the whole program),
    # etc.
    class CHANNEL_SPECIAL < Base
      def initialize(stream, name)
        super(stream, 0, 1)
        @name = name
      end

      def run(env)
        env.context.push(env.special_channel(@name))
      end
    end
  end
end