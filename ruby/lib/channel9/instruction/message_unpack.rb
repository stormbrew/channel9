module Channel9
  module Instruction
    # message_unpack first, remain, last
    # ---
    # Unpacks the argument list from the method package on the stack.
    # 
    # Takes the message at the top of the stack
    #  SP -> message
    # Pushes the message, then first+remain+last items on to the stack (described below):
    #  SP -> first_arg1 ... -> first_argN -> remain -> last_arg1 ... -> last_argN -> message
    #
    # If first is >0, unpacks first elements from the argument list
    # If remain is 1, pushes an array primitive of (total_count - last - first) elements from the arg list
    # If last >0, unpacks last elements from the argument list.
    #
    # If the number of arguments in the message is less than requested, the remaining will
    # be filled out with undefs.
    class MESSAGE_UNPACK < Base
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
        message = env.context.stack.last
        ary = message.positional

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
          if (pos >= ary.length)
            env.context.push(Primitive::Undef)
            pos -= 1
          else
            env.context.push(ary[pos])
            pos -= 1
          end
        end
      end
    end
  end
end