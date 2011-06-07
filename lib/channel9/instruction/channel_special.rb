module Channel9
  module Instruction
    class CHANNEL_SPECIAL < Base
      def initialize(stream, name)
        super(stream, 0, 1)
        @name = name
      end

      def run(env)
        env.context.push(env.special_channel(@name))
      end
    end
  end
end