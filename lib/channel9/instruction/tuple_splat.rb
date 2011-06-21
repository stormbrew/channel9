module Channel9
  module Instruction
    # tuple_splat
    # ---
    # Takes two tuples off the stack, appends the
    # contents of the second tuple to the first, and returns the
    # resulting tuple
    #
    # Takes message and tuple:
    #  SP -> tuple2 -> tuple1
    # Leaves the message on the stack:
    #  SP -> tuple_splatted
    #
    class TUPLE_SPLAT < Base
      def initialize(stream)
        super(stream, 2, 1)
      end

      def run(env)
        tuple2 = env.context.pop
        tuple1 = env.context.pop
        env.context.push(tuple1.c9_join(tuple2))
      end
    end
  end
end