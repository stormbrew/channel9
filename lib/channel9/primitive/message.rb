module Channel9
  module Primitive
    class Message < Base
      attr :name
      attr :positional
      attr :named

      def initialize(name, positional)
        @name = name
        @positional = positional
        @named = {}
      end

      def forward(name)
        Message.new(name, [@name, *@positional])
      end

      def to_s
        "<Channel9::Primitive::Message #{@name}(#{@positional.collect {|x| x.to_s }.join(', ')})>"
      end
    end
  end
end