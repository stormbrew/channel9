module Channel9
  module Instruction
    class SET_LOCAL < Base
      attr :local_name
      attr :local

      def initialize(stream, name)
        super(stream, 1, 0)
        @local_name = name
        @local = stream.local(name)
      end

      def run(environment)
        val = environment.context.pop
        environment.context.set_local(local_name, val)
      end
    end
  end
end