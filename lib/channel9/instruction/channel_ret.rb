module Channel9
  module Instruction
    class CHANNEL_RET < Base
      def initialize(stream)
        super(stream, 2)
      end

      def run(environment)
        channel = environment.context.pop
        value = environment.context.pop

        channel.dup.channel_send(environment, value, InvalidReturnChannel)
      end
    end
  end
end