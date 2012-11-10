module Channel9
  module Instruction
    # frame_get name
    # ---
    # Pushes the named frame variable onto the stack. 
    # A frame variable is one that is scoped to the
    # channel instance it's loaded in. This is useful for
    # things like return addresses and such that need to be
    # kept valid within the frame, even when the local scope
    # goes away. New channels inherit a copy of the old
    # channel's frame variable table.
    #
    # Takes no inputs.
    # After executing, the stack will look like:
    #  SP -> frame_value_or_nil
    class FRAME_GET < Base
      def initialize(stream, name)
        super(stream, 0, 1)
        @name = name
        @id = stream.framevar(name)
      end

      def arguments
        [@name]
      end
      def run(environment)
        val = environment.context.get_framevar(@id)
        environment.context.push(val)
      end
    end
  end
end