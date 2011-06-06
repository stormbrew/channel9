module Channel9
  module Instruction
    class PUSH < Base
      attr :value

      def initialize(stream, value)
        super(stream, 0, 1)
        @value = value
      end

      def run(environment)
        environment.context.push(@value)
      end
    end
  end
end