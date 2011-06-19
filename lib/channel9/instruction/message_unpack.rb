module Channel9
  module Instruction
    # message_unpack first, remain, last
    # ---
    # Unpacks the argument list from the method package on the stack.
    # 
    # Takes the message at the top of the stack
    #  SP -> message
    # Pushes the message, then first+remain+last items on to the stack (described below):
    #  SP -> message -> first_arg1 ... -> first_argN -> remain -> last_arg1 ... -> last_argN
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
        message = env.context.pop

        if (message.positional.length < @total)
          (@total - message.positional.length).times do
            message.positional << Primitive::Undef
          end
        end

        pos = 1
        @last.times do
          env.context.push(message.positional[@total - pos])
          pos += 1
        end

        if (@remain > 0)
          remain = []
          remain_count = message.positional.length - @first - @last
          remain_count.times do
            remain.push(message.positional[@total - pos])
            pos += 1
          end
          env.context.push(remain)
        end

        @first.times do
          env.context.push(message.positional[@total - pos])
          pos += 1
        end

        env.context.push(message)
      end
    end
  end
end