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
          elf = msg.positional.first
          ret.channel_send(elf.env, elf, InvalidReturnChannel)
        end
        klass.add_method(:define_method) do |cenv, msg, ret|
          elf, *args = msg.positional
          msg = Primitive::Message.new(msg.name, [], args)
          klass.channel_send(elf.env, msg, ret)
        end
        klass.add_method(:instance_variable_get) do |cenv, msg, ret|
          elf, name = msg.positional
          ret.channel_send(elf.env, elf.ivars[name], InvalidReturnChannel)
        end
        klass.add_method(:instance_variable_set) do |cenv, msg, ret|
          elf, name, val = msg.positional
          ret.channel_send(elf.env, elf.ivars[name] = val, InvalidReturnChannel)
        end
      end

      def initialize(env, klass = nil)
        @env = env
        @klass = klass.nil? ? env.special_channel[:Object] : klass
        @singleton = nil
        @ivars = {}
      end

      def to_c9
        self
      end

      def to_s
        "#<#{klass.name}:#{object_id}>"
      end

      def rebind(klass)
        @klass = klass
      end

      def singleton!
        @singleton ||= RubyClass.new(@env, self.to_s, nil)
      end

      def send_lookup(name)
        if (@singleton)
          res = @singleton.lookup(name)
          return res if !res.nil?
        end

        res = @klass.lookup(name)
        return res if !res.nil?

        return nil
      end

      def channel_send_with(elf, cenv, val, ret)
        if (val.is_a?(Primitive::Message))
          meth = send_lookup(val.name)
          if (meth.nil?)
            orig_name = val.name
            val = val.forward(:method_missing)
            meth = send_lookup(val.name)
            if (meth.nil?)
              raise "BOOM: No method_missing on #{elf}, orig message #{orig_name}"
            end
          end
          meth.channel_send(env, val.prefix(elf), ret)
        else
          raise "BOOM: Ruby object received unknown message #{val}."
        end
      end
      def channel_send(cenv, val, ret)
        channel_send_with(self, cenv, val, ret)
      end

      def truthy?
        true
      end
    end
  end
end