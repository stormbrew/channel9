module Channel9
  module Instruction
    # string_new count
    # ---
    # Takes +count+ strings from the stack and joins them into a 
    # new string object.
    #
    # Takes +count+ items from the top of the stack:
    #  SP -> item1 -> item2 -> item3 -> ... itemN
    # Pushes a new string onto the stack in their place:
    #  SP -> new_string
    class STRING_NEW < Base
      def initialize(stream, count)
        super(stream, count, 1)
        @count = count
      end

      def arguments
        [@count]
      end

      def run(env)
        str = ""
        @count.times do
          i = env.context.stack.pop
          if (i.is_a? Primitive::String)
            str << i.real_str
          else
            env.save_context do
              i.channel_send(env, Primitive::Message.new(@coerce, [], []), CallbackChannel.new {|cenv, val, ret|
                str << val.real_str
                throw :end_save
              })
            end
          end
        end
        env.context.push(Primitive::String.new(str))
      end
    end
  end
end