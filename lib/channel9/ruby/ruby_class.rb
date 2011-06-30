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
          elf = msg.system.first
          ret.channel_send(elf.env, RubyObject.new(elf.env, elf), InvalidReturnChannel)
        end
        klass.add_method(:new) do |cenv, msg, ret|
          elf, zuper, *sargs = msg.system
          args = msg.positional
          case elf
          when elf.env.special_channel[:Class]
            # special case for creating new classes.
            superklass, name = args
            ret.channel_send(elf.env, RubyClass.new(elf.env, name, superklass), InvalidReturnChannel)
          when elf.env.special_channel[:Module]
            # special case for creating new classes.
            name = args.first
            ret.channel_send(elf.env, RubyModule.new(elf.env, name), InvalidReturnChannel)
          when elf.env.special_channel[:Tuple]
            # special case for creating tuples
            ret.channel_send(elf.env, Tuple.new(elf.env, args.first), InvalidReturnChannel)
          else
            elf.channel_send(elf.env, Primitive::Message.new(:allocate, [], []), CallbackChannel.new {|cenv, obj, iret|
              obj.channel_send(elf.env, Primitive::Message.new(:initialize, [*sargs], args), CallbackChannel.new {|cenv, x, iret|
                ret.channel_send(elf.env, obj, InvalidReturnChannel)
              })
            })
          end
        end
        klass.add_method(:superclass) do |cenv, msg, ret|
          elf = msg.system.first
          ret.channel_send(elf.env, elf.superclass.to_c9, InvalidReturnChannel)
        end
        klass.add_method(:__c9_primitive_call__) do |cenv, msg, ret|
          elf = msg.system.first
          env = elf.respond_to?(:env) ? elf.env : cenv
          name, instance, *args = msg.positional
          imsg = Primitive::Message.new(name, [msg.system[2]], args)
          elf.channel_send_with(instance, nil, elf, env, imsg, ret)
        end

      end

      def initialize(env, name, superclass)
        super(env, env.special_channel[:Class])
        @name = name.nil? ? "Class:#{object_id}" : name
        @superclass = superclass.nil? ? env.special_channel[:Object] : superclass
        @instance_methods = {}
        @included = []
        @constant = {}
      end

      def singleton!
        @singleton ||= RubyClass.new(@env, self.to_s, self.superclass ? self.superclass.singleton! : self.klass)
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
      def del_method(name, stopper = false)
        if (stopper)
          @instance_methods[name.to_c9] = Primitive::Undef
        else
          @instance_methods.delete(name.to_c9)
        end
      end

      def lookup(name)
        name = name.to_c9
        [self, *@included.reverse].each do |mod|
          res = mod.instance_methods[name]
          return res, self if res
        end
        return @superclass.lookup(name) if !@superclass.nil? && @superclass != self
        return nil, nil
      end
    end
  end
end