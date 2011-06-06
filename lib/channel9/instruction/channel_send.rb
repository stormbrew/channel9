module Channel9
  module Instruction
    class CHANNEL_SEND < Base
      def initialize(stream)
        super(stream, 3, 0)
      end

      def run(environment)
        ret = environment.context.pop
        channel = environment.context.pop
        value = environment.context.pop

        channel.dup.channel_send(environment, value, ret)
      end
    end
  end
end