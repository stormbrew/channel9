module Channel9
  module Ruby
    class RubyObject
      attr :env
      attr :klass
      attr :singleton
      attr :ivars

      def self.object_klass(klass)
        klass.add_method(:method_missing) do |msg, ret|
          raise "BOOM: Method missing: #{msg.positional[1]}"
        end
        klass.add_method(:initialize) do |msg, ret|
          ret.channel_send(msg.positional.first, InvalidReturnChannel)
        end
        klass.add_method(:define_method) do |msg, ret|
          elf, *args = msg.positional
          msg = Primitive::Message.new(msg.name, args)
          klass.channel_send(msg, ret)
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

      def channel_send(val, ret)
        if (val.is_a?(Primitive::Message))
          meth = send_lookup(val.name)
          if (meth.nil?)
            orig_name = val.name
            val = val.forward(:method_missing)
            meth = send_lookup(val.name)
            if (meth.nil?)
              raise "BOOM: No method_missing on #{self}, orig message #{orig_name}"
            end
          end
          meth.channel_send(val.prefix(self), ret)
        else
          raise "BOOM: Ruby object received unknown message #{val}."
        end
      end
    end
  end
end