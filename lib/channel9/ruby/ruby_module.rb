module Channel9
  module Ruby
    class RubyModule < RubyObject
      attr :name
      attr :instance_methods
      attr :included
      attr :constant

      def self.module_klass(klass)
        klass.add_method(:const_set) do |msg, ret|
          elf, name, val = msg.positional
          elf.constant[name.to_c9] = val
          ret.channel_send(val.to_c9, InvalidReturnChannel)
        end
        klass.add_method(:const_get) do |msg, ret|
          elf, name = msg.positional
          val = elf.constant[name.to_c9]
          ret.channel_send(val.to_c9, InvalidReturnChannel)
        end
        klass.add_method(:include) do |msg, ret|
          elf, mod = msg.positional
          elf.include(mod)
          ret.channel_send(mod, InvalidReturnChannel)
        end
        klass.add_method(:define_method) do |msg, ret|
          elf, name, channel = msg.positional
          elf.add_method(name, channel)
          ret.channel_send(nil.to_c9, InvalidReturnChannel)
        end
        klass.add_method(:define_singleton_method) do |msg, ret|
          elf, name, channel = msg.positional
          elf.singleton!.add_method(name, channel)
          ret.channel_send(nil.to_c9, InvalidReturnChannel)
        end
      end

      def self.kernel_mod(mod)
        mod.add_method(:special_channel) do |msg, ret|
          elf, name = msg.positional
          ret.channel_send(env.special_channel[name], InvalidReturnChannel)
        end

        mod.add_method(:puts) do |msg, ret|
          elf, *strings = msg.positional
          puts(*strings)
          ret.channel_send(nil.to_c9, InvalidReturnChannel)
        end
        
        mod.add_method(:load) do |msg, ret|
          elf, path = msg.positional
          loader = elf.env.special_channel[:loader]
          stream = loader.compile(path.to_s)
          context = Channel9::Context.new(elf.env, stream)
          global_self = elf.env.special_channel[:global_self]
          context.channel_send(global_self, ret)
        end
      end

      def initialize(env, name)
        super(env, env.special_channel[:Module])
        @name = name
        @instance_methods = {}
        @included = []
      end

      def to_s
        "#<Channel9::Ruby::RubyModule: #{name}>"
      end

      def include(mod)
        @included.push(mod)
        @included.uniq!
      end

      def add_method(name, channel = nil, &cb)
        channel ||= CallbackChannel.new(&cb)
        @instance_methods[name.to_c9] = channel
      end
    end
  end
end