module Channel9
  module Primitive
    class MessageID
      def initialize(a)
        super(a.to_s)
      end

      def to_json(*a)
        {"message_id" => to_s}.to_json(*a)
      end
    end
    class ProtocolID
      def initialize(a)
        super(a.to_s)
      end

      def to_json(*a)
        {"protocol_id" => to_s}.to_json(*a)
      end
    end

    class Message
      attr :name
      attr :positional
      attr :system

      class <<self
        alias_method :channel_name, :name
      end

      def initialize(name, system, positional)
        @name = name
        @system = system
        @positional = positional
      end

      def forward(name)
        Message.new(name, @system, [@name, *@positional])
      end

      def prefix(*args)
        Message.new(@name, @system, args + @positional)
      end

      def splat(ary)
        Message.new(@name, @system, @positional + ary)
      end

      def to_s
        "<Channel9::Primitive::Message [#{@system.collect {|x| x.to_s }.join(', ')}].#{@name}(#{@positional.collect {|x| x.to_s }.join(', ')})>"
      end
    end
  end
end