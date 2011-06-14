module Channel9
  module Ruby
    class RubyClass < RubyObject
      attr :name
      attr :superclass
      attr :instance_methods
      attr :included
      attr :constant

      def self.class_klass(klass)
        klass.add_method(:allocate) do |msg, ret|
          elf = msg.positional.first
          ret.channel_send(RubyObject.new(elf.env, elf), InvalidReturnChannel)
        end
        klass.add_method(:new) do |msg, ret|
          elf, *args = msg.positional
          case elf
          when elf.env.special_channel[:Class]
            # special case for creating new classes.
            superklass, name = args
            ret.channel_send(RubyClass.new(elf.env, name, superklass), InvalidReturnChannel)
          when elf.env.special_channel[:Module]
            # special case for creating new classes.
            name = args.first
            ret.channel_send(RubyModule.new(elf.env, name), InvalidReturnChannel)
          else
            elf.channel_send(Primitive::Message.new(:allocate, [elf]), CallbackChannel.new {|obj, iret|
              obj.channel_send(Primitive::Message.new(:initialize, args), CallbackChannel.new {|x, iret|
                ret.channel_send(obj, InvalidReturnChannel)
              })
            })
          end
        end
      end

      def initialize(env, name, superclass)
        super(env, env.special_channel[:Class])
        @name = name
        @superclass = superclass.nil? ? env.special_channel[:Object] : superclass
        @instance_methods = {}
        @included = []
        @constant = {}
      end

      def to_s
        "#<Channel9::Ruby::RubyClass: #{name}>"
      end

      def rebind_super(superclass)
        @superclass = superclass
      end
      
      def include(mod)
        @included = @included + [mod] + mod.included
      end

      def add_method(name, channel = nil, &cb)
        channel ||= CallbackChannel.new(&cb)
        @instance_methods[name.to_c9] = channel
      end

      def lookup(name)
        name = name.to_c9
        #pp(:lookup_self=>self.to_s) if env.debug
        [self, *@included.reverse].each do |mod|
          #pp(:mod=>mod.to_s, :name=>name, :methods=>mod.instance_methods, :found=>mod.instance_methods[name]) if env.debug
          res = mod.instance_methods[name]
          return res if res
        end
        @superclass.lookup(name) if !@superclass.nil?
      end
    end
  end
end