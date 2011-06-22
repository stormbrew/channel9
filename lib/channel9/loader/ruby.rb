require 'ruby_parser'

require 'channel9'
require 'channel9/ruby'

module Channel9
  module Loader
    class Ruby
      attr :env

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

        env.special_channel[:loader] = self
        env.special_channel[:global_self] = Channel9::Ruby::RubyObject.new(env)
        c9rb_root = File.expand_path(File.dirname(__FILE__) + "../../../..")
        c9rb_env = c9rb_root + "/environment"
        globals = env.special_channel[:globals] = {
          :"$LOAD_PATH".to_c9 => [
            :"#{c9rb_root}/environment/kernel".to_c9, 
            :"#{c9rb_root}/environment/lib".to_c9, 
            :".".to_c9
          ]
        }
        globals[:"$:".to_c9] = globals[:"$LOAD_PATH".to_c9]
        env.special_channel[:unwinder] = Channel9::Ruby::Unwinder.new(env)
        
        object_klass.constant[:Object.to_c9] = object_klass
        object_klass.constant[:Module.to_c9] = module_klass
        object_klass.constant[:Class.to_c9] = class_klass
        object_klass.constant[:Kernel.to_c9] = kernel_mod

        # Builtin special types:
        [
          [:Fixnum, "Number"],
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
          ["singletons", "kernel", "symbol", "string", "enumerable",
           "tuple", "array", "exceptions"].each do |file|
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

      def set_argv(argv)
        object_klass = env.special_channel[:Object]
        object_klass.constant[:"ARGV".to_c9] = argv.to_c9
      end

      def channel_send(cenv, msg, ret)
        case msg.name
        when "compile"
          compile(msg.positional.first)
          ret.channel_send(env, true, InvalidReturnChannel)
        else
          raise "BOOM: Unknown message for loader: #{msg.name}."
        end
      end

      def compile(filename)
        begin
          File.open("#{filename}", "r") do |f|
            stream = Channel9::Stream.new
            stream.build do |builder|
              parser = RubyParser.new
              tree = parser.parse(f.read, filename)
              tree = [:file, tree]
              compiler = Channel9::Ruby::Compiler.new(builder)
              compiler.transform(tree)
            end
            return stream
          end
        rescue Errno::ENOENT, Errno::EISDIR
        end
        return nil
      end
    end
  end
end