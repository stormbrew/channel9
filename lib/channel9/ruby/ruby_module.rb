module Channel9
  module Ruby
    class RubyModule < RubyObject
      attr :name
      attr :instance_methods
      attr :included
      attr :constant

      def self.module_klass(klass)
        klass.add_method(:name) do |cenv, msg, ret|
          elf = msg.system.first
          ret.channel_send(elf.env, elf.name, InvalidReturnChannel)
        end
        klass.add_method(:scope_name) do |cenv, msg, ret|
          elf = msg.system.first
          if (elf != elf.env.special_channel(:Object))
            ret.channel_send(elf.env, elf.name.to_s + "::", InvalidReturnChannel)
          else
            ret.channel_send(elf.env, "", InvalidReturnChannel)
          end
        end
        klass.add_method(:==) do |cenv, msg, ret|
          elf = msg.system.first
          other = msg.positional.first
          ret.channel_send(cenv, elf.eql?(other), InvalidReturnChannel)
        end
        klass.add_method(:const_set) do |cenv, msg, ret|
          elf = msg.system.first
          name, val = msg.positional
          elf.constant[name.to_sym] = val
          ret.channel_send(elf.env, val, InvalidReturnChannel)
        end
        klass.add_method(:const_get) do |cenv, msg, ret|
          elf = msg.system.first
          name = msg.positional.first.to_sym
          val = elf.constant[name]
          ret.channel_send(elf.env, val, InvalidReturnChannel)
        end
        klass.add_method(:const_get_scoped) do |cenv, msg, ret|
          elf = msg.system.first
          name, next_scope = msg.positional

          found = elf.constant[name.to_sym]
          if (found)
            ret.channel_send(elf.env, found, InvalidReturnChannel)
          elsif (!next_scope.nil?)
            mod, next_scope = next_scope
            nmsg = Primitive::Message.new(:const_get_scoped, [], [name, next_scope])
            mod.channel_send(elf.env, nmsg, ret)
          else
            ret.channel_send(elf.env, nil, InvalidReturnChannel)
          end
        end
        klass.add_method(:const_defined?) do |cenv, msg, ret|
          elf = msg.system.first
          name = msg.positional.first.to_sym
          val = elf.constant[name] ? true : false
          ret.channel_send(elf.env, val, InvalidReturnChannel)
        end
        klass.add_method(:include) do |cenv, msg, ret|
          elf = msg.system.first
          mod = msg.positional.first
          elf.include(mod)
          ret.channel_send(elf.env, mod, InvalidReturnChannel)
        end
        klass.add_method(:define_method) do |cenv, msg, ret|
          elf, zuper, channel = msg.system
          name = msg.positional.first.to_sym
          elf.add_method(name, channel)
          ret.channel_send(elf.env, nil, InvalidReturnChannel)
        end
        klass.add_method(:remove_method) do |cenv, msg, ret|
          elf = msg.system.first
          name = msg.positional.first.to_sym
          elf.del_method(name, false)
          ret.channel_send(elf.env, nil, InvalidReturnChannel)
        end
        klass.add_method(:undef_method) do |cenv, msg, ret|
          elf = msg.system.first
          name = msg.positional.first.to_sym
          elf.del_method(name, true)
          ret.channel_send(elf.env, nil, InvalidReturnChannel)
        end
        klass.add_method(:alias_method) do |cenv, msg, ret|
          elf = msg.system.first
          new_name, orig_name = msg.positional
          elf.add_method(new_name.to_sym, elf.lookup(orig_name.to_sym)[0])
          ret.channel_send(elf.env, nil, InvalidReturnChannel)
        end
        klass.add_method(:method_defined?) do |cenv, msg, ret|
          elf = msg.system.first
          name = msg.positional.first.to_sym
          defined = elf.lookup(name)[0] ? true : false
          ret.channel_send(elf.env, defined, InvalidReturnChannel)
        end
        klass.add_method(:instance_method_prim) do |cenv, msg, ret|
          elf = msg.system.first
          name = msg.positional.first.to_sym
          meth = elf.lookup(name)[0]
          ret.channel_send(elf.env, meth, InvalidReturnChannel)
        end
      end

      def self.kernel_mod(mod)
        mod.add_method(:special_channel) do |cenv, msg, ret|
          elf = msg.system.first
          name = msg.positional.first
          ret.channel_send(elf.env, mod.env.special_channel(name.to_sym), InvalidReturnChannel)
        end

        mod.add_method(:print) do |cenv, msg, ret|
          elf = msg.system.first
          strings = msg.positional
          env = elf.respond_to?(:env)? elf.env : cenv
          strings.collect! do |s|
            if (!s.is_a? String)
              env.save_context do
                s.channel_send(env, Primitive::Message.new(:to_s_prim,[],[]), CallbackChannel.new {|ienv, sval, sret|
                  s = sval
                  throw :end_save
                })
              end
            end
            s
          end.flatten
          print(*strings)
          ret.channel_send(env, nil, InvalidReturnChannel)
        end
        
        mod.add_method(:raw_load) do |cenv, msg, ret|
          elf = msg.system.first
          path = msg.positional.first
          stream = Channel9::Loader::Ruby.compile(path)
          if (stream)
            context = Channel9::Context.new(elf.env, stream)
            global_self = elf.env.special_channel(:global_self)
            context.channel_send(elf.env, global_self, CallbackChannel.new {|ienv, imsg, iret|
              ret.channel_send(elf.env, true, InvalidReturnChannel)
            })
          else
            ret.channel_send(elf.env, false, InvalidReturnChannel)
          end
        end

        mod.add_method(:raise) do |cenv, msg, ret|
          elf = msg.system.first
          exc, desc, bt = msg.positional
          env = elf.respond_to?(:env)? elf.env : cenv
          loader = env.special_channel(:loader)
          constants = env.special_channel(:Object).constant

          if (exc.nil?)
            # re-raise current exception, or RuntimeError if none.
            exc = loader.globals[:'$!']
            if (exc.nil?)
              exc = constants[:RuntimeError]
            end
          end
          raise "BOOM: No RuntimeError!" if (exc.nil?)
          if (exc.kind_of?(String) || exc.klass == constants[:String])
            bt = desc
            desc = exc
            exc = constants[:RuntimeError]
          end
          if (bt.nil?)
            bt = make_backtrace(env.context)
          end

          exc.channel_send(env, Primitive::Message.new(:exception, [], [desc].compact), CallbackChannel.new {|ienv, exc, sret|
            raise_exception(env, exc, bt)
          })
        end

        mod.add_method(:respond_to?) do |cenv, msg, ret|
          elf = msg.system.first
          name = msg.positional.first
          if (elf.respond_to?(:env))
            ret.channel_send(elf.env, (!elf.send_lookup(name.to_sym).nil?), InvalidReturnChannel)
          else
            klass = cenv.special_channel(elf.class.name)
            ok = elf.respond_to?(:"c9_#{name}") || klass.lookup(name.to_sym)
            ret.channel_send(cenv, (!ok.nil?), InvalidReturnChannel)
          end
        end

        mod.add_method(:global_get) do |cenv, msg, ret|
          elf = msg.system.first
          name = msg.positional.first
          loader = elf.env.special_channel(:loader)
          ret.channel_send(elf.env, loader.globals[name.to_sym], InvalidReturnChannel)
        end
        mod.add_method(:global_set) do |cenv, msg, ret|
          elf = msg.system.first
          name, val = msg.positional
          loader = elf.env.special_channel(:loader)
          loader.globals[name.to_sym] = val
          ret.channel_send(elf.env, val, InvalidReturnChannel)
        end
        mod.add_method(:to_s) do |cenv, msg, ret|
          elf = msg.system.first
          if (elf.respond_to?(:env))
            ret.channel_send(elf.env, :"#<#{elf.klass.name}:#{elf.object_id}>", InvalidReturnChannel)
          else
            ret.channel_send(cenv, elf.to_s, InvalidReturnChannel)
          end
        end
        mod.add_method(:to_s_prim) do |cenv, msg, ret|
          elf = msg.system.first
          env = elf.respond_to?(:env)? elf.env : cenv
          if (elf.is_a?(String) || elf.is_a?(Symbol))
            ret.channel_send(env, elf, InvalidReturnChannel)
          else
            elf.channel_send(env, Primitive::Message.new(:to_s, [], []), CallbackChannel.new {|ienv, val, iret|
              if (val.is_a?(String) || val.is_a?(Symbol))
                ret.channel_send(env, val, InvalidReturnChannel)
              else
                val.channel_send(env, Primitive::Message.new(:to_s_prim, [], []), CallbackChannel.new {|ienv, ival, iret|
                  ret.channel_send(env, ival, InvalidReturnChannel)
                })
              end
            })
          end
        end

        mod.add_method(:caller) do |cenv, msg, ret|
          ctx = cenv.context
          bt = make_backtrace(ctx)
          ret.channel_send(cenv, bt, InvalidReturnChannel)
        end
      end

      def self.channel9_mod(mod)
        mod.singleton!.add_method(:prim_dir_glob) do |cenv, msg, ret|
          glob = msg.positional.first
          res = Dir.glob(glob).to_a
          ret.channel_send(cenv, res, InvalidReturnChannel)
        end

        mod.singleton!.add_method(:prim_sprintf) do |cenv, msg, ret|
          format, *args = msg.positional
          ret.channel_send(cenv, sprintf(format.to_s, *args), ret)
        end

        mod.singleton!.add_method(:prim_time_now) do |cenv, msg, ret|
          ret.channel_send(cenv, Time.now.to_f, InvalidReturnChannel)
        end

        mod.singleton!.add_method(:prim_stat) do |cenv, msg, ret|
          elf = msg.system.first
          name = msg.positional.first
          begin
            info = File.stat(name)
            info_arr = [
              info.atime.to_i,
              info.blksize,
              info.blockdev?,
              info.blocks,
              info.chardev?,
              info.ctime.to_i,
              info.dev,
              info.dev_major,
              info.dev_minor,
              info.directory?,
              info.executable?,
              info.executable_real?,
              info.file?,
              info.ftype.to_sym,
              info.gid,
              info.grpowned?,
              info.ino,
              info.mode,
              info.mtime.to_i,
              info.nlink,
              info.owned?,
              info.pipe?,
              info.rdev,
              info.rdev_major,
              info.rdev_minor,
              info.readable?,
              info.readable_real?,
              info.setgid?,
              info.setuid?,
              info.size,
              info.socket?,
              info.sticky?,
              info.symlink?,
              info.uid,
              info.writable?,
              info.writable_real?,
              info.zero?
            ].collect {|i| i }
            ret.channel_send(cenv, info_arr, InvalidReturnChannel)
          rescue Errno::ENOENT
            ret.channel_send(cenv, nil, InvalidReturnChannel)
          end
        end
      end

      def initialize(env, name)
        super(env, env.special_channel(:Module))
        @name = name.nil? ? "Module:#{object_id}" : name
        @instance_methods = {}
        @included = []
        @constant = {}
      end

      def to_s
        "#<Channel9::Ruby::RubyModule: #{name}>"
      end

      def self.make_backtrace(ctx)
        bt = []
        while (!ctx.nil?)
          bt.unshift(ctx.line_info.join(":"))

          ctx = ctx.caller
        end
        bt
      end
      def self.raise_exception(env, exc, bt)
        loader = env.special_channel(:loader)
        handler = env.special_channel(:unwinder).handlers.pop
        loader.globals[:"$!"] = exc
        exc.channel_send(env, Primitive::Message.new(:set_backtrace, [], [bt]), CallbackChannel.new {
          handler.channel_send(env, Primitive::Message.new(:raise, [], [exc]), InvalidReturnChannel)
        })
      end

      def include(mod)
        @included.push(mod)
        @included.uniq!
      end

      def lookup(name)
        return @instance_methods[name], self
      end

      def add_method(name, channel = nil, &cb)
        raise "BOOM: Invalid channel #{channel}" if !channel.kind_of?(Context) && !cb
        channel ||= CallbackChannel.new(&cb)
        @instance_methods[name] = channel
      end
      def del_method(name, stopper = false)
        if (stopper)
          @instance_methods[name] = Primitive::Undef
        else
          @instance_methods.delete(name)
        end
      end
    end
  end
end