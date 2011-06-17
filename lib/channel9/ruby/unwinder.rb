module Channel9
  module Ruby
    class TerminalUnwinder
      def self.channel_send(env, msg, ret)
        raise "BOOM: Uncaught unwind: #{msg}."
      end
    end

    class Unwinder
      def initialize(env)
        @env = env
        @cur_handler = TerminalUnwinder
      end

      def channel_send(cenv, msg, ret)
        if (msg.name == :get)
          ret.channel_send(@env, @cur_handler, InvalidReturnChannel)
        elsif (msg.name == :set)
          old_handler = @cur_handler
          @cur_handler = msg.positional.first
          ret.channel_send(@env, old_handler, InvalidReturnChannel)
        else
          raise "BOOM: Unknown method on Unwinder #{msg.name}."
        end
      end
    end
  end
end