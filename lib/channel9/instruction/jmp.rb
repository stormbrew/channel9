module Channel9
  module Instruction
    class JMP < Base
      attr :to

      def initialize(stream, to)
        super(stream)
        @to = to
      end

      def run(environment)
        environment.context.set_pos(@to)
      end
    end
  end
end