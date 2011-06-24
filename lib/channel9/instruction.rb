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
        "#{instruction_name} #{arguments.join(' ')}"
      end

      def instruction_name
        cname = self.class.name
        cname = cname.gsub(/^.+::([^:]+)$/, '\1').downcase
        cname
      end

      # override in derived classes if they have extra args
      def arguments
        []
      end

      def to_json(*a)
        # instructions should appear as one entity.
        if (a.first.is_a? JSON.state)
          a = a.dup
          state = a.shift.dup
          state.array_nl = ""
          state.indent = ""
          state.space = ""
          a.unshift(state)
        end

        [instruction_name, *arguments].to_json(*a)
      end
      def to_s
        "#{instruction_name} #{arguments.join(', ')}"
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
