module Channel9
  module Instruction
    # local_set name
    # ---
    # Takes the top element off the stack and stores it in the named
    # local variable in the current execution context..
    #
    # Takes one element from the stack:
    #  SP -> element
    # After executing, the element will be removed from the stack.
    class LOCAL_SET < Base
      attr :name
      attr :id

      def initialize(stream, name)
        super(stream, 1, 0)
        @name = name
      end

      def arguments
        [@name]
      end
    end
  end
end