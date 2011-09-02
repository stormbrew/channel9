require 'ruby_parser'

require 'channel9'
require 'channel9/ruby'

module Channel9
  module Loader
    class Ruby
      attr :env
      attr :globals

      def to_c9
        self
      end

      def initialize(debug = false)
        @env = Channel9::Environment.new(debug)

        object_klass = Channel9::Ruby::RubyClass.new(env, "Object", nil)
        env.set_special_channel(:Object, object_klass)

        module_klass = Channel9::Ruby::RubyClass.new(env, "Module", object_klass)
        env.set_special_channel(:Module, module_klass)

        class_klass = Channel9::Ruby::RubyClass.new(env, "Class", module_klass)
        env.set_special_channel(:Class, class_klass)
        object_klass.rebind(class_klass)
        module_klass.rebind(class_klass)
        class_klass.rebind(class_klass)

        Channel9::Ruby::RubyObject.object_klass(object_klass)
        Channel9::Ruby::RubyModule.module_klass(module_klass)
        Channel9::Ruby::RubyClass.class_klass(class_klass)

        kernel_mod = Channel9::Ruby::RubyModule.new(env, "Kernel")
        env.set_special_channel(:Kernel, kernel_mod)
        Channel9::Ruby::RubyModule.kernel_mod(kernel_mod)
        object_klass.include(kernel_mod)

        channel9_mod = Channel9::Ruby::RubyModule.new(env, "Channel9")
        env.set_special_channel(:Channel9, channel9_mod)
        Channel9::Ruby::RubyModule.channel9_mod(channel9_mod)

        tuple_klass = Channel9::Ruby::RubyClass.new(env, "Channel9::Tuple", object_klass)
        env.set_special_channel(:Tuple, tuple_klass)
        Channel9::Ruby::Tuple.tuple_klass(tuple_klass)

        env.set_special_channel(:loader, self)
        env.set_special_channel(:global_self, Channel9::Ruby::RubyObject.new(env))
        c9rb_root = File.expand_path(File.dirname(__FILE__) + "../../../..")
        c9rb_env = c9rb_root + "/environment"
        @globals = {
          :"$LOAD_PATH" => [
            :"#{c9rb_root}/environment/kernel", 
            :"#{c9rb_root}/environment/lib",
            :"#{c9rb_root}/environment/site-lib",
            :"."
          ]
        }
        globals[:"$:"] = globals[:"$LOAD_PATH"]
        env.set_special_channel(:unwinder, Channel9::Ruby::Unwinder.new(env))
        
        object_klass.constant[:Object] = object_klass
        object_klass.constant[:Module] = module_klass
        object_klass.constant[:Class] = class_klass
        object_klass.constant[:Kernel] = kernel_mod
        object_klass.constant[:Channel9] = channel9_mod
        channel9_mod.constant[:Tuple] = tuple_klass

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
          [:StaticTuple, "Tuple"],
          [:Table, "Table"],
          [:Message, "Message"],
          [:TrueClass, "TrueC"],
          [:FalseClass, "FalseC"],
          [:NilClass, "NilC"],
          [:UndefClass, "UndefC"]
        ].each do |ruby_name, c9_name|
          klass = Channel9::Ruby::RubyClass.new(env, ruby_name, object_klass)
          env.set_special_channel("Channel9::Primitive::#{c9_name}", klass)
          object_klass.constant[ruby_name.to_sym] = klass
        end

        env.no_debug do
          ["singletons", "string", "kernel", "symbol", "enumerable",
           "static_tuple", "tuple", "array", "proc", "exceptions"].each do |file|
            file = :"#{c9rb_env}/kernel/alpha/#{file}.rb"
            object_klass.channel_send(env,
              Primitive::Message.new(:raw_load, [], [file]),
              CallbackChannel.new {}
            )
          end
          object_klass.channel_send(env,
            Primitive::Message.new(:raw_load, [], [:"#{c9rb_env}/kernel/beta.rb"]),
            CallbackChannel.new {}
          )
        end
      end

      def setup_environment(exe, argv)
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
        else
          raise "BOOM: Unknown message for loader: #{msg.name}."
        end
      end

      def self.compile_string(type, str, filename = "__eval__", line = 1)
        stream = Channel9::Stream.new
        stream.build do |builder|
          parser = RubyParser.new
          begin
            tree = parser.parse(str, filename)
          rescue Racc::ParseError => e
            puts "parse error in #{filename}: #{e}"
            return nil
          rescue SyntaxError => e
            puts "syntax error in #{filename}: #{e}"
            return nil
          end
          tree = s(type.to_sym, tree)
          tree.filename = filename
          tree.line = line
          compiler = Channel9::Ruby::Compiler.new(builder)
          compiler.transform(tree)
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
    end
  end
end