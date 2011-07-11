module Channel9
  module Ruby
    class TerminalUnwinder
      def self.channel_send(env, msg, ret)
        if (msg.name == :raise)
          exc = msg.positional.first
          global = env.special_channel(:Channel9)
          global.channel_send(env, Primitive::Message.new(:uncaught_exception,[],[exc]), CallbackChannel.new {|ienv, bt, iret|
            exit(1)
          })
        else
          raise "BOOM: Uncaught unwind of unknown kind: #{msg}."
        end
      end
    end

    # The Unwinder channel is a stack of unwinders. It can take
    # three kinds of message:
    #  - A message object pops the top (current) unwinder and sends
    #    the message to it.
    #  - nil pops the current unwiner and returns it.
    #  - Anything else pushes the current unwinder onto the stack and
    #    returns the old one.
    #
    # So you set an unwind handler by sending it. After the scope
    # in which the unwind handler is valid is done, you send nil to
    # remove it. In order to initiate an unwind, you send the unwinder
    # a message.
    class Unwinder
      attr :handlers
      def initialize(env)
        @env = env
        @handlers = [TerminalUnwinder]
      end

      def channel_send(cenv, set, ret)
        case set
        when Primitive::Message
          @handlers.pop.channel_send(@env, set, ret)
        when nil
          ret.channel_send(@env, @handlers.pop, InvalidReturnChannel)
        else
          old_handler = handlers.last
          @handlers.push(set)
          ret.channel_send(@env, old_handler, InvalidReturnChannel)
        end
      end
    end
  end
end