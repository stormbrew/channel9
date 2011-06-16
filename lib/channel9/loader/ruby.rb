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
        env.special_channel[:globals] = {
          :"$LOAD_PATH" => ["environment/kernel", "environment/lib", "."]
        }

        object_klass.constant[:Object] = object_klass
        object_klass.constant[:Module] = module_klass
        object_klass.constant[:Class] = class_klass
        object_klass.constant[:Kernel] = kernel_mod

        object_klass.channel_send(
          Primitive::Message.new(:load, [], ["boot.rb"]),
          CallbackChannel.new {}
        )
      end

      def channel_send(msg, ret)
        case msg.name
        when "compile"
          compile(msg.positional.first)
          ret.channel_send(true, InvalidReturnChannel)
        else
          raise "BOOM: Unknown message for loader: #{msg.name}."
        end
      end

      def compile(filename)
        env.special_channel[:globals][:"$LOAD_PATH"].each do |path|
          begin
            File.open("#{path}/#{filename}", "r") do |f|
              stream = Channel9::Stream.new
              stream.build do |builder|
                tree = RubyParser.new.parse(f.read)
                tree = [:file, tree]
                Channel9::Ruby::Compiler.transform(builder, tree)
              end
              return stream
            end
          rescue Errno::ENOENT
          end
        end
        raise LoadError, "Could not find #{filename} in $LOAD_PATH"
      end
    end
  end
end