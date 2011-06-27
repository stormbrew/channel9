require 'ruby_parser'

require 'channel9'
require 'channel9/ruby'

module Channel9
  module Loader
    class Ruby
      attr :env

      def to_c9
        self
      end

      def initialize(debug = false)
        @env = Channel9::Environment.new(debug)

        object_klass = Channel9::Ruby::RubyClass.new(env, "Object", nil)
        env.special_channel[:Object] = object_klass
        Channel9::Ruby::RubyObject.object_klass(object_klass)

        module_klass = Channel9::Ruby::RubyClass.new(env, "Module", object_klass)
        env.special_channel[:Module] = module_klass
        Channel9::Ruby::RubyModule.module_klass(module_klass)

        class_klass = Channel9::Ruby::RubyClass.new(env, "Class", module_klass)
        env.special_channel[:Class] = class_klass
        Channel9::Ruby::RubyClass.class_klass(class_klass)
        
        object_klass.rebind(class_klass)
        module_klass.rebind(class_klass)
        class_klass.rebind(class_klass)

        kernel_mod = Channel9::Ruby::RubyModule.new(env, "Kernel")
        env.special_channel[:Kernel] = kernel_mod
        Channel9::Ruby::RubyModule.kernel_mod(kernel_mod)
        object_klass.include(kernel_mod)

        channel9_mod = Channel9::Ruby::RubyModule.new(env, "Channel9")
        env.special_channel[:Channel9] = channel9_mod
        Channel9::Ruby::RubyModule.channel9_mod(channel9_mod)

        env.special_channel[:loader] = self
        env.special_channel[:global_self] = Channel9::Ruby::RubyObject.new(env)
        c9rb_root = File.expand_path(File.dirname(__FILE__) + "../../../..")
        c9rb_env = c9rb_root + "/environment"
        globals = env.special_channel[:globals] = {
          :"$LOAD_PATH".to_c9 => [
            :"#{c9rb_root}/environment/kernel".to_c9, 
            :"#{c9rb_root}/environment/lib".to_c9,
            :"#{c9rb_root}/environment/site-lib".to_c9,
            :".".to_c9
          ]
        }
        globals[:"$:".to_c9] = globals[:"$LOAD_PATH".to_c9]
        env.special_channel[:unwinder] = Channel9::Ruby::Unwinder.new(env)
        
        object_klass.constant[:Object.to_c9] = object_klass
        object_klass.constant[:Module.to_c9] = module_klass
        object_klass.constant[:Class.to_c9] = class_klass
        object_klass.constant[:Kernel.to_c9] = kernel_mod
        object_klass.constant[:Channel9.to_c9] = channel9_mod

        env.set_error_handler do |err, ctx|
          puts "Unhandled VM Error in #{self}"
          pp Channel9::Ruby::RubyModule.make_backtrace(ctx).collect {|i| i.real_str }
          pp ctx.debug_info
          raise err
        end

        # Builtin special types:
        [
          [:Fixnum, "Number"],
          [:Float, "Float"],
          [:Symbol, "String"],
          [:Tuple, "Tuple"],
          [:Table, "Table"],
          [:Message, "Message"],
          [:TrueClass, "TrueC"],
          [:FalseClass, "FalseC"],
          [:NilClass, "NilC"],
          [:UndefClass, "UndefC"]
        ].each do |ruby_name, c9_name|
          klass = Channel9::Ruby::RubyClass.new(env, ruby_name.to_c9, object_klass)
          env.special_channel["Channel9::Primitive::#{c9_name}"] = klass
          object_klass.constant[ruby_name.to_c9] = klass
        end

        env.no_debug do
          ["singletons", "string", "kernel", "symbol", "enumerable",
           "tuple", "array", "proc", "exceptions"].each do |file|
            file = :"#{c9rb_env}/kernel/alpha/#{file}.rb".to_c9
            object_klass.channel_send(env,
              Primitive::Message.new(:raw_load, [], [file]),
              CallbackChannel.new {}
            )
          end
          object_klass.channel_send(env,
            Primitive::Message.new(:raw_load, [], [:"#{c9rb_env}/kernel/beta.rb".to_c9]),
            CallbackChannel.new {}
          )
        end
      end

      def setup_environment(exe, argv)
        c9_mod = env.special_channel[:Channel9]
        argv = argv.collect {|i| Primitive::String.new(i) }.to_c9
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
          begin
            compiled = compile_string(type.to_sym, str.real_str, filename.real_str, line.real_num)
            callable = CallableContext.new(cenv, compiled)
          rescue => e
            puts "Compile error: #{e}"
          end
          ret.channel_send(env, callable, InvalidReturnChannel)
        else
          raise "BOOM: Unknown message for loader: #{msg.name}."
        end
      end

      def compile_string(type, str, filename = "__eval__", line = 1)
        stream = Channel9::Stream.new
        stream.build do |builder|
          parser = RubyParser.new
          begin
            tree = parser.parse(str, filename)
          rescue Racc::ParseError => e
            puts "parse error in #{filename}"
            raise
          end
          tree = s(type.to_sym, tree)
          tree.filename = filename
          tree.line = line
          compiler = Channel9::Ruby::Compiler.new(builder)
          compiler.transform(tree)
        end
        return stream
      end

      def compile(filename)
        begin
          File.open("#{filename}", "r") do |f|
            return compile_string(:file, f.read, filename)
          end
        rescue Errno::ENOENT, Errno::EISDIR
        rescue => e
          puts "Compile error: #{e}"
        end
        return nil
      end
    end
  end
end