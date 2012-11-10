module Channel9
  module Instruction
    # tuple_unpack first, remain, last
    # ---
    # Unpacks the values in the given tuple as specified.
    # 
    # Takes the tuple at the top of the stack
    #  SP -> tuple
    # Pushes the tuple, then first+remain+last items on to the stack (described below):
    #  SP -> first_arg1 ... -> first_argN -> remain -> last_arg1 ... -> last_argN -> tuple
    #
    # If first is >0, unpacks first elements from the argument list
    # If remain is 1, pushes an array primitive of (total_count - last - first) elements from the arg list
    # If last >0, unpacks last elements from the argument list.
    #
    # If the number of arguments in the tuple is less than requested, the remaining will
    # be filled out with nils.
    class TUPLE_UNPACK < Base
      def initialize(stream, first, remain, last)
        total = first + (remain > 0? 1 : 0) + last
        super(stream, 1, 1 + total)
        @first = first
        @remain = remain
        @last = last
        @total = total
      end

      def arguments
        [@first, @remain, @last]
      end

      def run(env)
        tuple = env.context.stack.last
        ary = tuple.real_ary.dup

        pos = ary.length - 1
        @last.times do
          if (pos >= @first)
            env.context.push(ary.pop)
            pos -= 1
          end
        end

        if (@remain > 0)
          remain = []

          while (pos >= @first)
            remain.unshift(ary.pop)
            pos -= 1
          end
          env.context.push(remain)
        end

        pos = @first - 1

        while (pos >= 0)
          env.context.push(ary[pos])
          pos -= 1
        end
      end
    end
  end
end