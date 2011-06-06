module Channel9
  module Instruction
    class Base
      attr :stream

      def initialize(stream, stack_input = 0, stack_output = 0)
        @stream = stream
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
