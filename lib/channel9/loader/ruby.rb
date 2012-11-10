require 'ruby_parser'

require 'channel9'
require 'channel9/ruby'

module Channel9
  module Loader
    class Ruby
      attr :env
      attr :globals
      attr :root
      attr :root_environment

      def to_c9
        self
      end

      def initialize()
        @root = File.expand_path(File.dirname(__FILE__) + "../../../..")
        @root_environment = @root + "/environment"
      end

      def setup_environment(exe, argv)
        env.set_special_channel(:set_special_channel, CallbackChannel.new {|ienv, val, iret|
          ienv.set_special_channel(val.positional[0].to_sym, val.positional[1])
          iret.channel_send(ienv, val.positional[1], InvalidReturnChannel)
        })

        env.set_special_channel(:loader, self)
        env.set_special_channel(:terminal_unwinder, Channel9::Ruby::TerminalUnwinder)

        ["basic_hash", "object", "class", "module", "finish"].each do |file|
          alpha = Channel9::Stream.from_json(File.read(@root_environment + "/kernel/alpha/#{file}.c9b"))
          alpha_context = Channel9::Context.new(@env, alpha)
          @env.save_context do
            alpha_context.channel_send(@env, nil, CallbackChannel.new {|ienv, val, iret|
            })
          end
        end

        ["singletons", "module", "string", "symbol", "kernel", "enumerable",
         "static_tuple", "tuple", "array", "proc", "exceptions", "class"].each do |file|
          file = "#{@root_environment}/kernel/beta/#{file}.c9b"
          load_c9_file(file)
        end

        env.set_special_channel(:initial_load_path, [
            :"#{@root}/environment/kernel",
            :"#{@root}/environment/lib",
            :"#{@root}/environment/site-lib",
            :"."
          ])
        env.set_special_channel(:print, CallbackChannel.new {|ienv, val, iret|
          print(val.positional.first.to_s)
          iret.channel_send(@env, val, InvalidReturnChannel)
        })

        load_c9_file("#{@root_environment}/kernel/delta.c9b")

        setup_channel9_mod

        c9_mod = env.special_channel(:Channel9)
        argv = argv
        env.no_debug do
          env.save_context do
            c9_mod.channel_send(env,
              Primitive::Message.new(:setup_environment, [], [exe, argv]),
              CallbackChannel.new { throw :end_save}
            )
          end
        end
      end

      def channel_send(cenv, msg, ret)
        case msg.name.to_sym
        when :compile
          type, str, filename, line = msg.positional
          callable = nil
          compiled = Ruby.compile_string(type.to_sym, str.to_s, filename.to_s, line)
          if (compiled)
            callable = Channel9::Context.new(cenv, compiled)
            ret.channel_send(env, callable, InvalidReturnChannel)
          else
            ret.channel_send(env, nil, InvalidReturnChannel)
          end
        when :load
          path = msg.positional.first#; puts path
          stream = Channel9::Loader::Ruby.compile(path)
          if (stream)
            context = Channel9::Context.new(env, stream)
            global_self = env.special_channel(:global_self)
            context.channel_send(env, global_self, CallbackChannel.new {|ienv, imsg, iret|
              ret.channel_send(env, true, InvalidReturnChannel)
            })
          else
            ret.channel_send(env, false, InvalidReturnChannel)
          end
        when :load_c9
          path = msg.positional.first
          ret.channel_send(env, load_c9_file(@root_environment + "/" + path), InvalidReturnChannel)
        when :backtrace
          ctx = env.current_context
          bt = []
          while (!ctx.nil?)
            bt.push(ctx.line_info.join(":"))

            ctx = ctx.caller
          end
          ret.channel_send(env, bt, InvalidReturnChannel)
        else
          raise "BOOM: Unknown message for loader: #{msg.name}."
        end
      end

      def load_file(path)
        stream = Channel9::Loader::Ruby.compile(path)
        if (stream)
          context = Channel9::Context.new(env, stream)
          global_self = env.special_channel(:global_self)
          context.channel_send(env, global_self, CallbackChannel.new {|ienv, imsg, iret| })
          return true
        else
          return false
        end
      end

      def load_c9_file(path)
        code = Channel9::Stream.from_json(File.read(path))
        code_context = Channel9::Context.new(@env, code)
        @env.save_context do
          code_context.channel_send(@env, nil, CallbackChannel.new {|ienv, val, iret|
            return val
          })
        end
      end

      def self.compile_string(type, str, filename = "__eval__", line = 1)
        stream = Channel9::Stream.new
        stream.build do |builder|
          parser = RubyParser.new
          begin
            tree = parser.parse(str, filename)
            tree = s(type.to_sym, tree)
            tree.file = filename
            tree.line = line
            compiler = Channel9::Ruby::Compiler.new(builder)
            compiler.transform(tree)
          rescue Racc::ParseError => e
            puts "parse error in #{filename}: #{e}"
            return nil
          rescue ArgumentError => e
            puts "argument error in #{filename}: #{e}"
            return nil
          rescue SyntaxError => e
            puts "syntax error in #{filename}: #{e}"
            return nil
          rescue NotImplementedError => e
            puts "not implemented error in #{filename}: #{e}"
            return nil
          rescue RegexpError => e
            puts "invalid regex error in #{filename}: #{e}"
            return nil
          end
        end
        return stream
      end

      def self.compile(filename)
        begin
          File.open("#{filename}", "r") do |f|
            return compile_string(:file, f.read, filename)
          end
        rescue Errno::ENOENT, Errno::EISDIR
          return nil
        end
      end

      def add_method_to_mod(mod, name, &methblock)
        @env.save_context do
          msgid = Primitive::MessageID.new(name.to_s)
          meth = CallbackChannel.new(&methblock)
          mod.channel_send(env, Primitive::Message.new(:"ruby_sys:make_singleton", [],[]), CallbackChannel.new {|_, singleton, _|
            singleton.channel_send(env, Primitive::Message.new(:"ruby_sys:add_method", [], [msgid, meth]), CallbackChannel.new {
              return
            })
          })
        end
      end

      def setup_channel9_mod
        mod = env.special_channel(:Channel9)
        add_method_to_mod(mod, :prim_dir_glob) do |cenv, msg, ret|
          glob = msg.positional.first
          res = Dir.glob(glob).to_a
          ret.channel_send(cenv, res, InvalidReturnChannel)
        end

        add_method_to_mod(mod, :prim_sprintf) do |cenv, msg, ret|
          format, *args = msg.positional
          ret.channel_send(cenv, sprintf(format.to_s, *args), ret)
        end

        add_method_to_mod(mod, :prim_time_now) do |cenv, msg, ret|
          ret.channel_send(cenv, Time.now.to_f, InvalidReturnChannel)
        end

        add_method_to_mod(mod, :prim_stat) do |cenv, msg, ret|
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
    end
  end
end
