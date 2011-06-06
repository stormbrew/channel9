module Channel9
  module Instruction
    class GET_LOCAL < Base
      def initialize(stream, name)
        super(stream, 0, 1)
        @name = name
      end
      def run(environment)
        val = environment.context.get_local(@name)
        environment.context.push(val)
      end
    end
  end
end