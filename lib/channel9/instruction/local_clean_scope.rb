module Channel9
  module Instruction
    # local_clean_scope
    # ---
    # Creates a clean scope for local variables, hiding all
    # variables from the original context.
    #
    # Takes nothing from the stack and pushes nothing back on.
    class LOCAL_CLEAN_SCOPE < Base
      def initialize(stream)
        super(stream, 0, 0)
      end

      def run(environment)
        environment.context.clean_scope
      end
    end
  end
end