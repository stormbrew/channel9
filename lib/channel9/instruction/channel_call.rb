module Channel9
  module Instruction
    class CHANNEL_CALL < Base
      def initialize(stream)
        super(stream, 2)
      end

      # assumes the channel return is the next instruction (aka, the state of the current context)..
      def run(environment)
        channel = environment.context.pop
        value = environment.context.pop

        channel.dup.channel_send(environment, value, environment.context.dup)
      end
    end
  end
end