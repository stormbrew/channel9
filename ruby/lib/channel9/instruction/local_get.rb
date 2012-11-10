module Channel9
  module Instruction
    # local_get name
    # ---
    # Pushes the named local variable onto the stack. 
    # A local variable is one that is scoped to the
    # channel instance it's loaded in. This is useful for
    # things like return addresses and such that need to be
    # kept valid within the local, even when the local scope
    # goes away. New channels do not inherit a copy of the old
    # channel's local variable table.
    #
    # Takes no inputs.
    # After executing, the stack will look like:
    #  SP -> local_value_or_nil
    class LOCAL_GET < Base
      def initialize(stream, name)
        super(stream, 0, 1)
        @name = name
      end

      def arguments
        [@name]
      end
    end
  end
end