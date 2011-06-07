module Channel9
  module Instruction
    class Base
      attr :stream
      attr :stack_input
      attr :stack_output

      def initialize(stream, stack_input = 0, stack_output = 0)
        @stream = stream
        @stack_input = stack_input
        @stack_output = stack_output
      end

      def debug_info
        self.class.name
      end
    end

    def self.get(name)
      begin
        require "channel9/instruction/#{name}"
        const_get(name.to_s.upcase)
      rescue LoadError, NameError => e
        nil
      end
    end
  end
end
