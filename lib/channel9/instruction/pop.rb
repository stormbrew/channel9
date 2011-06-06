module Channel9
  module Instruction
    class POP < Base
      def initialize(stream)
        super(stream, 1, 0)
      end
      def run(environment)
        environment.context.pop
      end
    end
  end
end