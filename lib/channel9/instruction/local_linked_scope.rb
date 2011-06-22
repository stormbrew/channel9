module Channel9
  module Instruction
    # local_linked_scope
    # ---
    # Creates a new scope for local variables, while keeping
    # a reference to the variable scope originally attached
    # to this context. You can then get/set locals from the
    # enclosing scope by using a depth parameter to the
    # local_[get|set] instructions.
    #
    # Takes nothing from the stack and pushes nothing back on.
    class LOCAL_LINKED_SCOPE < Base
      def initialize(stream)
        super(stream, 0, 0)
      end

      def run(environment)
        environment.context.linked_scope
      end
    end
  end
end