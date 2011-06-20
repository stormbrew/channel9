module Channel9
  module Ruby
    class RubyObject
      attr :env
      attr :klass
      attr :singleton
      attr :ivars

      def self.object_klass(klass)
        klass.add_method(:method_missing) do |cenv, msg, ret|
          raise "BOOM: Method missing: #{msg.positional[1]}"
        end
        klass.add_method(:initialize) do |cenv, msg, ret|
          elf = msg.system.first
          ret.channel_send(elf.env, elf, InvalidReturnChannel)
        end
        klass.add_method(:class) do |cenv, msg, ret|
          elf = msg.system.first
          if (elf.respond_to?(:env))
            ret.channel_send(elf.env, elf.klass, InvalidReturnChannel)
          else
            ret.channel_send(cenv, cenv.special_channel[elf.class.name], InvalidReturnChannel)
          end
        end
        klass.add_method(:object_id) do |cenv, msg, ret|
          elf = msg.system.first
          env = elf.respond_to?(:env)? elf.env : cenv
          ret.channel_send(env, elf.object_id, InvalidReturnChannel)
        end
        klass.add_method(:define_method) do |cenv, msg, ret|
          elf = msg.system.first
          args = msg.positional
          msg = Primitive::Message.new(msg.name, [], args)
          elf.klass.channel_send(elf.env, msg, ret)
        end
        klass.add_method(:instance_variable_get) do |cenv, msg, ret|
          elf = msg.system.first
          name = msg.positional.first
          ret.channel_send(elf.env, elf.ivars[name], InvalidReturnChannel)
        end
        klass.add_method(:instance_variable_set) do |cenv, msg, ret|
          elf = msg.system.first
          name, val = msg.positional
          ret.channel_send(elf.env, elf.ivars[name] = val, InvalidReturnChannel)
        end
        klass.add_method(:equal?) do |cenv, msg, ret|
          elf = msg.system.first
          other = msg.positional.first
          if (elf.respond_to?(:env))
            ret.channel_send(elf.env, elf.equal?(other).to_c9, InvalidReturnChannel)
          else
            ret.channel_send(cenv, (elf == other).to_c9, InvalidReturnChannel)
          end
        end
        klass.add_method(:singleton) do |cenv, msg, ret|
          elf = msg.system.first
          ret.channel_send(elf.env, elf.singleton, InvalidReturnChannel)
        end

        klass.add_method(:singleton!) do |cenv, msg, ret|
          elf = msg.system.first
          ret.channel_send(elf.env, elf.singleton!, InvalidReturnChannel)
        end
      end

      def initialize(env, klass = nil)
        @env = env
        @klass = klass.nil? ? env.special_channel[:Object] : klass
        @singleton = nil
        @ivars = {}
      end

      def to_s
        "#<Channel9::Ruby::RubyObject: #{@klass.name}:#{object_id}>"
      end

      def to_c9
        self
      end

      def to_c9_str
        @env.save_context do
          channel_send(@env, Primitive::Message.new(:to_s_prim, [], []), CallbackChannel.new {|ienv, val, iret|
            return val
          })
        end
      end

      def rebind(klass)
        @klass = klass
      end

      def singleton!
        @singleton ||= RubyClass.new(@env, self.to_s, self.klass)
      end

      def send_lookup(name)
        if (@singleton)
          pp(:singleton=>@singleton.to_s) if (env.debug)
          res = @singleton.lookup(name)
          return res if !res.nil?
        end

        pp(:klass=>@klass.to_s) if (env.debug)
        res = @klass.lookup(name)
        return res if !res.nil?

        return nil
      end

      def channel_send_with(elf, singleton, klass, cenv, val, ret)
        if (val.is_a?(Primitive::Message))
          meth = (singleton && singleton.lookup(val.name)) || klass.lookup(val.name)
          if (meth.nil?)
            orig_name = val.name
            val = val.forward(:method_missing)
            meth = (singleton && singleton.lookup(val.name)) || klass.lookup(val.name)
            if (meth.nil?)
              raise "BOOM: No method_missing on #{elf}, orig message #{orig_name}"
            end
          end
          val = Primitive::Message.new(val.name, [elf] + val.system, val.positional)
          meth.channel_send(env, val, ret)
        else
          raise "BOOM: Ruby object received unknown message #{val}."
        end
      end
      def channel_send(cenv, val, ret)
        channel_send_with(self, singleton, klass, cenv, val, ret)
      end

      def truthy?
        true
      end
    end
  end
end