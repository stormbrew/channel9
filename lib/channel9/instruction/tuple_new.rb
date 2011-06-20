module Channel9
  module Instruction
    # tuple_new count
    # ---
    # Takes +count+ items from the stack and creates a tuple
    # out of them.
    #
    # Takes +count+ items from the top of the stack:
    #  SP -> item1 -> item2 -> item3 -> ... itemN
    # Pushes a new tuple onto the stack in their place:
    #  SP -> new_tuple
    class TUPLE_NEW < Base
      def initialize(stream, count)
        super(stream, count, 1)
        @count = count
      end

      def arguments
        [@count]
      end

      def run(env)
        arr = []
        @count.times do
          arr << env.context.stack.pop
        end
        env.context.push(Primitive::Tuple.new(arr))
      end
    end
  end
end