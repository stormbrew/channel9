module Channel9
  module Instruction
    # frame_set name
    # ---
    # Takes the top element off the stack and stores it in the named
    # frame variable in the current execution context..
    #
    # Takes one element from the stack:
    #  SP -> element
    # After executing, the element will be removed from the stack.
    class FRAME_SET < Base
      attr :name
      attr :id

      def initialize(stream, name)
        super(stream, 1, 0)
        @name = name
        @id = stream.framevar(name)
      end

      def arguments
        [@name]
      end

      def run(environment)
        val = environment.context.pop
        environment.context.set_framevar(@id, val)
      end
    end
  end
end