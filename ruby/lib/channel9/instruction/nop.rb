module Channel9
  module Instruction
    # nop
    # ---
    # Does nothing.
    class NOP < Base
      def run(environment)
      end
    end
  end
end