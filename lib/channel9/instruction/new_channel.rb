require 'channel9/channel'

module Channel9
  module Instruction
    class NEW_CHANNEL < Base
      attr :label

      def initialize(stream, label)
        super(stream, 0, 1)
        @label = label
      end

      def run(environment)
        channel = Channel.new(environment.context, @label)
        environment.context.push(channel)
      end
    end
  end
end