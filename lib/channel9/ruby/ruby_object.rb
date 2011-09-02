module Channel9
  module Ruby
    class RubyObject
      attr :env
      attr :klass
      attr :singleton
      attr :ivars

      def self.object_klass(klass)
        klass.add_method(:initialize) do |cenv, msg, ret|
          elf = msg.system.first
          ret.channel_send(elf.env, elf, InvalidReturnChannel)
        end
        klass.add_method(:class) do |cenv, msg, ret|
          elf = msg.system.first
          if (elf.respond_to?(:klass))
            ret.channel_send(elf.env, elf.klass, InvalidReturnChannel)
          else
            ret.channel_send(cenv, cenv.special_channel(elf.class.channel_name), InvalidReturnChannel)
          end
        end
        klass.add_method(:object_id) do |cenv, msg, ret|
          elf = msg.system.first
          env = elf.respond_to?(:env)? elf.env : cenv
          ret.channel_send(env, elf.object_id, InvalidReturnChannel)
        end
        klass.add_method(:define_method) do |cenv, msg, ret|
          elf, zuper, *system = msg.system
          msg = Primitive::Message.new(msg.name, system, msg.positional)
          elf.klass.channel_send(elf.env, msg, ret)
        end
        klass.add_method(:define_singleton_method) do |cenv, msg, ret|
          elf, zuper, channel = msg.system
          name = msg.positional.first.to_sym
          elf.singleton!.add_method(name, channel)
          ret.channel_send(elf.env, nil, InvalidReturnChannel)
        end
        klass.add_method(:instance_eval_prim) do |cenv, msg, ret|
          elf, zuper, channel = msg.system
          msg = Primitive::Message.new(:__instance_eval___, [elf], msg.positional)
          raise "BOOM: Not a callablechannel! #{channel}" if !channel.kind_of?(Channel9::Context)
          channel.channel_send(cenv, msg, ret)
        end
        klass.add_method(:instance_variable_get) do |cenv, msg, ret|
          elf = msg.system.first
          name = msg.positional.first
          ret.channel_send(elf.env, elf.ivars[name.to_sym], InvalidReturnChannel)
        end
        klass.add_method(:instance_variable_set) do |cenv, msg, ret|
          elf = msg.system.first
          name, val = msg.positional
          ret.channel_send(elf.env, elf.ivars[name.to_sym] = val, InvalidReturnChannel)
        end
        klass.add_method(:instance_variable?) do |cenv, msg, ret|
          elf = msg.system.first
          name = msg.positional.first
          found = elf.ivars.key?(name.to_sym)
          ret.channel_send(elf.env, found, InvalidReturnChannel)
        end
        klass.add_method(:instance_variables_prim) do |cenv, msg, ret|
          elf = msg.system.first
          if (elf.respond_to? :env)
            found = elf.ivars.keys.collect {|i| i }
            ret.channel_send(elf.env, found, InvalidReturnChannel)
          else
            # Primitives don't have ivars. For now.
            ret.channel_send(cenv, [], InvalidReturnChannel)
          end
        end
        klass.add_method(:equal?) do |cenv, msg, ret|
          elf = msg.system.first
          other = msg.positional.first
          if (elf.respond_to?(:env))
            ret.channel_send(elf.env, elf.equal?(other), InvalidReturnChannel)
          else
            ret.channel_send(cenv, (elf == other), InvalidReturnChannel)
          end
        end
        klass.add_method(:singleton) do |cenv, msg, ret|
          elf = msg.system.first
          if (elf.respond_to? :env)
            ret.channel_send(elf.env, elf.singleton, InvalidReturnChannel)
          else
            ret.channel_send(cenv, nil, InvalidReturnChannel)
          end
        end

        klass.add_method(:singleton!) do |cenv, msg, ret|
          elf = msg.system.first
          if (elf.respond_to? :env)
            ret.channel_send(elf.env, elf.singleton!, InvalidReturnChannel)
          else
            object = cenv.special_channel(:Object)
            type_error = object.constant[:TypeError]
            msg = "no virtual class for primitive types"
            elf.channel_send(cenv, Primitive::Message.new(:raise, [], [type_error, msg]), InvalidReturnChannel)
          end
        end

        klass.add_method(:dup) do |cenv, msg, ret|
          elf = msg.system.first
          ret.channel_send(elf.env, elf.make_dup, InvalidReturnChannel)
        end

        klass.add_method(:send_prim) do |cenv, msg, ret|
          elf, zuper, *sargs = msg.system
          name, *args = msg.positional
          msg = Primitive::Message.new(name.to_sym, [*sargs], args)
          elf.channel_send(cenv, msg, ret)
        end
      end

      def initialize(env, klass = nil)
        @env = env
        @klass = klass.nil? ? env.special_channel(:Object) : klass
        @singleton = nil
        @ivars = {}
      end

      def to_s
        # disable debugging for this call so we
        # don't infinite loop trying to to_s debug output
        # during the to_s.
        env.no_debug do
          "#<Channel9::Ruby::RubyObject(#{@klass.name})>"#: #{to_c9_str}>"
        end
      end

      def to_c9
        self
      end

      def to_c9_str
        @env.save_context do
          channel_send(@env, Primitive::Message.new(:to_sym, [], []), CallbackChannel.new {|ienv, val, iret|
            return val
          })
        end
      end

      def make_dup
        duped = dup
        @env.save_context do
          duped.channel_send(@env, Primitive::Message.new(:initialize_copy, [], [self]), CallbackChannel.new {|ienv, val, iret|
            return duped
          })
        end
      end
      def initialize_copy(other)
        @singleton = nil
        @ivars = @ivars.dup
      end
      def rebind(klass)
        @klass = klass
      end

      def singleton!
        @singleton ||= RubyClass.new(@env, self.to_s, self.klass)
      end

      def send_lookup(name)
        (@singleton && singleton.lookup(name)) || @klass.lookup(name)
      end

      def make_super(elf, klass, msg)
        CallbackChannel.new do |ienv, imsg, iret|
          if (imsg.nil?)
            imsg = msg
          else
            msg = Primitive::Message.new(imsg.name, [imsg.system[2]], imsg.positional)
          end
          elf.channel_send_with(elf, nil, klass.superclass, ienv, msg, iret)
        end
      end

      def channel_send_with(elf, singleton, klass, cenv, val, ret)
        if (val.is_a?(Primitive::Message))
          name = val.name.to_sym
          env.for_debug do
            pp(:trace=>{:name=>val.name.to_s, :ret=>ret.to_s, :self => elf.to_s, :klass=>klass.to_s, :singleton=>singleton.to_s, :system => val.system.collect {|i| i.to_s },:args => val.positional.collect {|i| i.to_s }})
          end
          meth, found_klass = (singleton && singleton.lookup(name)) || klass.lookup(name)
          if (meth.nil?)
            orig_name = name
            val = val.forward(name = :method_missing)
            meth, found_klass = (singleton && singleton.lookup(name)) || klass.lookup(name)
            if (meth.nil?)
              raise "BOOM: No method_missing on #{elf}, orig message #{orig_name}"
            end
          end
          env.for_debug do
            pp(:lookup=>{:name=>val.name.to_s, :found => meth.to_s, :on => found_klass.to_s, :ret => ret.to_s })
          end
          super_call = make_super(elf, found_klass, val)
          val = Primitive::Message.new(val.name, [elf, super_call] + val.system, val.positional)
          meth.channel_send(env, val, ret)
        else
          raise "BOOM: Ruby object received unknown message #{val}."
        end
      end
      def channel_send(cenv, val, ret)
        if (val.kind_of? Primitive::Message)
          channel_send_with(self, singleton, klass, cenv, val, ret)
        else
          case val.to_s
          when /^@[^@]/
            ret.channel_send(env, @ivars[val.to_sym], InvalidReturnChannel)
          when /^_*[A-Z]/
            ret.channel_send(env, @constant[val.to_sym], InvalidReturnChannel)
          end
        end
      end

      def truthy?
        true
      end
    end
  end
end