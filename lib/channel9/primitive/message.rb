module Channel9
  module Primitive
    class Message
      attr :name
      attr :positional
      attr :named

      def initialize(name, positional)
        @name = name
        @positional = positional
      end

      def channel_send(environment, val, ret)
        ret.channel_send(environment, nil, InvalidReturnChannel)
      end
    end
  end
end