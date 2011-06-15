module Channel9
  module Primitive
    class Message < Base
      attr :name
      attr :positional
      attr :system

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

      def to_s
        "<Channel9::Primitive::Message #{@name}(#{@positional.collect {|x| x.to_s }.join(', ')})>"
      end
    end
  end
end