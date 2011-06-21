module Channel9
  module Ruby
    class RubyModule < RubyObject
      attr :name
      attr :instance_methods
      attr :included
      attr :constant

      def self.module_klass(klass)
        klass.add_method(:const_set) do |cenv, msg, ret|
          elf = msg.system.first
          name, val = msg.positional
          elf.constant[name.to_c9] = val
          ret.channel_send(elf.env, val.to_c9, InvalidReturnChannel)
        end
        klass.add_method(:const_get) do |cenv, msg, ret|
          elf = msg.system.first
          name = msg.positional.first
          val = elf.constant[name.to_c9]
          ret.channel_send(elf.env, val.to_c9, InvalidReturnChannel)
        end
        klass.add_method(:const_get_scoped) do |cenv, msg, ret|
          elf = msg.system.first
          names = msg.positional.dup
          finding = names.pop
          last = elf
          scopes = [elf] + names.collect do |name|
            last.constant[name.to_c9]
          end
          const = nil
          scopes.reverse.each do |scope|
            if (const = scope && scope.constant[finding.to_c9])
              break
            end
          end
          ret.channel_send(elf.env, const.to_c9, InvalidReturnChannel)
        end
        klass.add_method(:include) do |cenv, msg, ret|
          elf = msg.system.first
          mod = msg.positional.first
          elf.include(mod)
          ret.channel_send(elf.env, mod, InvalidReturnChannel)
        end
        klass.add_method(:define_method) do |cenv, msg, ret|
          elf = msg.system.first
          name, channel = msg.positional
          elf.add_method(name, channel)
          ret.channel_send(elf.env, nil.to_c9, InvalidReturnChannel)
        end
        klass.add_method(:define_singleton_method) do |cenv, msg, ret|
          elf = msg.system.first
          name, channel = msg.positional
          elf.singleton!.add_method(name, channel)
          ret.channel_send(elf.env, nil.to_c9, InvalidReturnChannel)
        end
      end

      def self.kernel_mod(mod)
        mod.add_method(:special_channel) do |cenv, msg, ret|
          elf = msg.system.first
          name = msg.positional.first
          ret.channel_send(elf.env, mod.env.special_channel[name], InvalidReturnChannel)
        end

        mod.add_method(:print) do |cenv, msg, ret|
          elf = msg.system.first
          strings = msg.positional
          env = elf.respond_to?(:env)? elf.env : cenv
          strings.collect! do |s|
            env.save_context do
              s.channel_send(env, Primitive::Message.new(:to_s_prim,[],[]), CallbackChannel.new {|ienv, sval, sret|
                s = sval
                throw :end_save
              })
            end
            s
          end.flatten
          print(*strings)
          ret.channel_send(elf.env, nil.to_c9, InvalidReturnChannel)
        end
        
        mod.add_method(:raw_load) do |cenv, msg, ret|
          elf = msg.system.first
          path = msg.positional.first
          loader = elf.env.special_channel[:loader]
          path.channel_send(elf.env, Primitive::Message.new(:to_s_prim,[],[]), CallbackChannel.new {|ienv, sval, sret|
            stream = loader.compile(sval.to_s)
            if (stream)
              context = Channel9::Context.new(elf.env, stream)
              global_self = elf.env.special_channel[:global_self]
              context.channel_send(elf.env, global_self, CallbackChannel.new {|ienv, imsg, iret|
                ret.channel_send(elf.env, true.to_c9, InvalidReturnChannel)
              })
            else
              ret.channel_send(elf.env, false.to_c9, InvalidReturnChannel)
            end
          })
        end

        mod.add_method(:raise) do |cenv, msg, ret|
          elf = msg.system.first
          exc, desc, bt = msg.positional
          env = elf.respond_to?(:env)? elf.env : cenv
          globals = env.special_channel[:globals]
          constants = env.special_channel[:Object].constant

          if (exc.nil?)
            # re-raise current exception, or RuntimeError if none.
            exc = globals[:'$!'.to_c9]
            if (exc.nil?)
              exc = constants[:RuntimeError.to_c9]
            end
          end
          raise "BOOM: No RuntimeError!" if (exc.nil?)
          if (exc.kind_of?(Primitive::String) || exc.klass == constants[:String.to_c9])
            bt = desc
            desc = exc
            exc = constants[:RuntimeError.to_c9]
          end

          exc.channel_send(env, Primitive::Message.new(:exception, [], [desc].compact), CallbackChannel.new {|ienv, exc, sret|
            handler = env.special_channel[:unwinder].handlers.pop
            globals[:"$!".to_c9] = exc
            handler.channel_send(env, Primitive::Message.new(:raise, [], [exc]), InvalidReturnChannel)
          })
        end

        mod.add_method(:respond_to?) do |cenv, msg, ret|
          elf = msg.system.first
          name = msg.positional.first
          if (elf.respond_to?(:env))
            ret.channel_send(elf.env, (!elf.send_lookup(name.to_sym).nil?).to_c9, InvalidReturnChannel)
          else
            klass = cenv.special_channel[elf.class.name]
            ok = elf.respond_to?(:"c9_#{name}") || klass.lookup(name.to_sym)
            ret.channel_send(cenv, (!ok.nil?).to_c9, InvalidReturnChannel)
          end
        end

        mod.add_method(:global_get) do |cenv, msg, ret|
          elf = msg.system.first
          name = msg.positional.first
          globals = elf.env.special_channel[:globals]
          ret.channel_send(elf.env, globals[name].to_c9, InvalidReturnChannel)
        end
        mod.add_method(:global_set) do |cenv, msg, ret|
          elf = msg.system.first
          name, val = msg.positional
          globals = elf.env.special_channel[:globals]
          globals[name] = val
          ret.channel_send(elf.env, val, InvalidReturnChannel)
        end
        mod.add_method(:to_s) do |cenv, msg, ret|
          elf = msg.system.first
          if (elf.respond_to?(:env))
            ret.channel_send(elf.env, :"#<#{elf.klass.name}:#{elf.object_id}>".to_c9, InvalidReturnChannel)
          else
            ret.channel_send(cenv, Primitive::String.new(elf.to_s), InvalidReturnChannel)
          end
        end
        mod.add_method(:to_s_prim) do |cenv, msg, ret|
          elf = msg.system.first
          env = elf.respond_to?(:env)? elf.env : cenv
          if (elf.is_a?(Primitive::String))
            ret.channel_send(env, elf, InvalidReturnChannel)
          else
            elf.channel_send(env, Primitive::Message.new(:to_s, [], []), CallbackChannel.new {|ienv, val, iret|
              val.channel_send(env, Primitive::Message.new(:to_primitive_str, [], []), CallbackChannel.new {|ienv, ival, iret|
                ret.channel_send(env, ival, InvalidReturnChannel)
              })
            })
          end
        end
      end

      def initialize(env, name)
        super(env, env.special_channel[:Module])
        @name = name
        @instance_methods = {}
        @included = []
        @constant = {}
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