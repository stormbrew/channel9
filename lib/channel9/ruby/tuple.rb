module Channel9
  module Ruby
    # Provides an interface for obtaining slabs of mutable
    # memory for lists of objects. Modeled after rubinius' tuple.
    class Tuple < RubyObject
      attr :data
      attr :read_only
      attr :length

      def self.tuple_klass(klass)
        klass.add_method(:at) {|cenv, msg, ret|
          elf = msg.system.first
          pos = msg.positional.first
          ret.channel_send(elf.env, elf.data[pos.real_num], InvalidReturnChannel)
        }
        klass.add_method(:put) {|cenv, msg, ret|
          elf = msg.system.first
          pos, val = msg.positional
          ret.channel_send(elf.env, elf.data[pos.real_num] = val, InvalidReturnChannel)
        }
        klass.add_method(:length) {|cenv, msg, ret|
          elf = msg.system.first
          ret.channel_send(elf.env, elf.length.to_c9, InvalidReturnChannel)
        }
        klass.add_method(:subary) {|cenv, msg, ret|
          elf = msg.system.first
          first, last = msg.positional
          ret.channel_send(elf.env, elf.data[first.real_num...last.real_num].to_c9, InvalidReturnChannel)
        }
        klass.add_method(:to_tuple_prim) {|cenv, msg, ret|
          elf = msg.system.first
          ret.channel_send(elf.env, Primitive::Tuple.new(elf.data), InvalidReturnChannel)
        }
      end

      def initialize(env, count)
        super(env, env.special_channel[:Tuple])
        @length = count.real_num
        @data = Array.new(count.real_num, Primitive::Nil)
      end
    end
  end
end