module Channel9
  module Instruction
    # jmp label
    # ---
    # Unconditionally jumps to the label specified.
    #
    # Takes no inputs and produces no outputs.
    class JMP < Base
      attr :to

      def initialize(stream, to)
        super(stream)
        @to = to
      end

      def arguments
        [@to]
      end

      def run(environment)
        environment.context.set_pos(@to)
      end
    end
  end
end