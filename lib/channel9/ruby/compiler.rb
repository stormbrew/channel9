module Channel9
  module Ruby
    module Compiler
      def self.transform_self(builder)
        builder.local_get("self")
      end

      def self.transform_lit(builder, literal)
        builder.push literal
      end
      def self.transform_str(builder, str)
        builder.push str.intern # TODO: make this build a ruby string class instead
      end

      def self.transform_while(builder, condition, body, unk)
        begin_label = builder.make_label("while.begin")
        done_label = builder.make_label("while.done")

        builder.set_label(begin_label)
        transform(builder, condition)
        builder.jmp_if_not(done_label)
        
        transform(builder, body)

        builder.jmp(begin_label)
        builder.set_label(done_label)
      end

      def self.transform_lasgn(builder, name, val)
        transform(builder, val)
        builder.local_set(name)
      end
      def self.transform_lvar(builder, name)
        builder.local_get(name)
      end

      def self.transform_block(builder, *lines)
        lines.each do |line|
          transform(builder, line)
        end
      end

      def self.transform_call(builder, target, method, arglist)
        if (target.nil?)
          transform_self(builder)
        else
          transform(builder, target)
        end

        _, *arglist = arglist
        arglist.each do |arg|
          transform(builder, arg)
        end

        builder.message_new(method.to_c9, arglist.length)
        builder.channel_call
        builder.pop
      end

      def self.transform_file(builder, body)
        builder.local_set("return")
        builder.local_set("self")
        transform(builder, body)
        builder.local_get("return")
        builder.local_get("self")
        builder.channel_ret
      end

      def self.transform(builder, tree)
        name, *info = tree
        send(:"transform_#{name}", builder, *info)
      end
    end
  end
end