module Channel9
  module Instruction
    class SPECIAL_CHANNEL < Base
      def initialize(stream, name)
        super(stream, 0, 1)
        @name = name
        @channel = self.class.const_get(name).new
      end

      def run(env)
        env.context.push(@channel)
      end

      class InvalidReturn
        def channel_send(environment, val, ret)
          raise "Invalid Return, exiting"
        end
      end

      class Stdout
        def channel_send(environment, val, ret)
          $stdout.puts(val)
          ret.channel_send(environment, val, InvalidReturn)
        end
      end
    end
  end
end