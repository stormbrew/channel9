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

      module InvalidReturnChannel
        def self.channel_send(environment, val, ret)
          raise "Invalid Return, exiting"
        end
      end
    end
  end
end