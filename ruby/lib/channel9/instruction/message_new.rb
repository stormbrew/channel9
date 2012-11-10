module Channel9
  module Instruction
    # message_new name, sys_count, count
    # ---
    # Pushes a message onto the stack, with count
    # positional arguments consumed and stored in the argument
    # pack, along with the message name.
    #
    # Takes count inputs as well as a message name:
    #  SP ->arg1 -> arg2 ... -> argN -> sys_arg1 -> sys_arg2 ... -> sys_argN
    # Leaves the message on the stack:
    #  SP -> message
    #
    # Messages are one of the primitive types in channel9.
    # They facilitate sending messages that will be dispatched on
    # name (or other traits) to an object (represented by a channel).
    # All objects in channel9 are channels, and messages are the
    # primative by which they communicate.
    class MESSAGE_NEW < Base
      def initialize(stream, name, sys_count, count)
        super(stream, sys_count + count, 1)
        @name = name
        @sys_count = sys_count
        @count = count
      end

      def arguments
        [@name, @sys_count, @count]
      end

      def run(env)
        sys_args = []
        args = []
        @count.times { args.unshift(env.context.pop) }
        @sys_count.times { sys_args.unshift(env.context.pop) }
        message = Primitive::Message.new(@name, sys_args, args)
        env.context.push(message)
      end
    end
  end
end