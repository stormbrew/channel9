module Channel9
  module Ruby
    class RubyClass < RubyObject
      attr :name
      attr :superclass
      attr :instance_methods
      attr :included
      attr :constant

      def self.class_klass(klass)
        klass.add_method(:allocate) do |cenv, msg, ret|
          elf = msg.positional.first
          ret.channel_send(elf.env, RubyObject.new(elf.env, elf), InvalidReturnChannel)
        end
        klass.add_method(:new) do |cenv, msg, ret|
          elf, *args = msg.positional
          case elf
          when elf.env.special_channel[:Class]
            # special case for creating new classes.
            superklass, name = args
            ret.channel_send(elf.env, RubyClass.new(elf.env, name, superklass), InvalidReturnChannel)
          when elf.env.special_channel[:Module]
            # special case for creating new classes.
            name = args.first
            ret.channel_send(elf.env, RubyModule.new(elf.env, name), InvalidReturnChannel)
          else
            elf.channel_send(elf.env, Primitive::Message.new(:allocate, [], [elf]), CallbackChannel.new {|cenv, obj, iret|
              obj.channel_send(elf.env, Primitive::Message.new(:initialize, [], args), CallbackChannel.new {|cenv, x, iret|
                ret.channel_send(elf.env, obj, InvalidReturnChannel)
              })
            })
          end
        end
        klass.add_method(:superclass) do |cenv, msg, ret|
          elf = msg.positional.first
          ret.channel_send(elf.env, elf.superclass, InvalidReturnChannel)
        end
        klass.add_method(:__c9_primitive_call__) do |cenv, msg, ret|
          elf, name, instance, *args = msg.positional
          imsg = Primitive::Message.new(name, msg.system, args)
          elf.channel_send_with(instance, nil, elf, elf.env, imsg, ret)
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

      def singleton!
        if (self == env.special_channel[:Class])
          @singleton ||= RubyClass.new(@env, self.to_s, self.klass)
          @singleton.rebind_super(@singleton)
        else
          super
        end
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
        pp(:lookup_self=>self.to_s, :super=>@superclass.to_s) if env.debug
        [self, *@included.reverse].each do |mod|
          pp(:mod=>mod.to_s, :name=>name, :methods=>mod.instance_methods.collect {|n, z| [n,z.to_s] }, :found=>mod.instance_methods[name].to_s) if env.debug
          res = mod.instance_methods[name]
          return res if res
        end
        @superclass.lookup(name) if !@superclass.nil? && @superclass != self
      end
    end
  end
end