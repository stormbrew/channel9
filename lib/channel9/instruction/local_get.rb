module Channel9
  module Instruction
    # local_get depth, name
    # ---
    # Pushes the named local onto the stack. 
    #
    # Takes no inputs.
    # After executing, the stack will look like:
    #  SP -> local_value_or_nil
    #
    # Gets the named local from the current context's variable scope
    # and pushes it onto the stack.
    # If depth is >0, pulls it from an enclosing scope's local scope.
    class LOCAL_GET < Base
      def initialize(stream, depth, name)
        super(stream, 0, 1)
        @name = name
        @depth = depth
        @id = stream.local(name)
      end

      def arguments
        [@depth, @name]
      end
      def run(environment)
        val = environment.context.get_local(@depth, @id)
        environment.context.push(val)
      end
    end
  end
end