module Channel9
  module Instruction
    # message_new name, count
    # ---
    # Pushes a message onto the stack, with count
    # positional arguments consumed and stored in the argument
    # pack, along with the message name.
    #
    # Takes count inputs as well as a message name:
    #  SP -> arg1 -> arg2 ... -> argN
    # Leaves the message on the stack:
    #  SP -> message
    #
    # Messages are one of the primitive types in channel9.
    # They facilitate sending messages that will be dispatched on
    # name (or other traits) to an object (represented by a channel).
    # All objects in channel9 are channels, and messages are the
    # primative by which they communicate.
    class MESSAGE_NEW < Base
      def initialize(stream, name, count)
        super(stream, count, 1)
        @name = name
        @count = count
      end

      def run(env)
        args = []
        @count.times { args.unshift(env.context.pop) }
        message = Primitive::Message.new(@name, args)
        env.context.push(message)
      end
    end
  end
end