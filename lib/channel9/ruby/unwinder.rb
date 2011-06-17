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

      def channel_send(cenv, set, ret)
        if (set.nil?)
          ret.channel_send(@env, @cur_handler, InvalidReturnChannel)
        else
          old_handler = @cur_handler
          @cur_handler = set
          ret.channel_send(@env, old_handler, InvalidReturnChannel)
        end
      end
    end
  end
end