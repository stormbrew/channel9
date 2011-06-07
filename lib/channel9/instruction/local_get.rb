module Channel9
  module Instruction
    # local_get name
    # ---
    # Pushes the named local onto the stack. 
    #
    # Takes no inputs.
    # After executing, the stack will look like:
    #  SP -> local_value_or_nil
    #
    # Gets the named local from the current context's variable scope
    # and pushes it onto the stack.
    class LOCAL_GET < Base
      def initialize(stream, name)
        super(stream, 0, 1)
        @name = name
      end
      def run(environment)
        val = environment.context.get_local(@name)
        environment.context.push(val)
      end
    end
  end
end