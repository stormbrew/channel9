module Channel9
  module Instruction
    class NEW_CHANNEL < Base
      attr :label

      def initialize(stream, label)
        super(stream, 0, 1)
        @label = label
      end

      def run(environment)
        channel = environment.context.dup.set_pos(@label)
        environment.context.push(channel)
      end
    end
  end
end