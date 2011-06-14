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

      def self.transform_if(builder, cond, truthy, falsy)
        falsy_label = builder.make_label("if.falsy")
        done_label = builder.make_label("if.done")

        transform(builder, cond)
        builder.jmp_if_not(falsy_label)

        transform(builder, truthy)
        builder.jmp(done_label)

        builder.set_label(falsy_label)
        transform(builder, falsy) if (falsy)

        builder.set_label(done_label)
      end

      def self.transform_args(builder, *args)
        # TODO: Support splats and such.
        builder.message_unpack(args.count + 1, 0, 0)
        builder.pop # message stays on stack for further manipulation, we're done with it.
        args.reverse.each do |arg|
          builder.local_set(arg)
        end
        builder.local_set("self")
      end

      def self.transform_scope(builder, block)
        transform(builder, block)
      end

      def self.transform_defn(builder, name, args, code)
        label_prefix = "method:#{name}"
        method_label = builder.make_label(label_prefix + ".body")
        method_done_label = builder.make_label(label_prefix + ".done")

        builder.jmp(method_done_label)
        builder.set_label(method_label)
        builder.local_clean_scope
        builder.local_set("return")
        transform(builder, args)
        transform(builder, code)

        builder.local_get("return")
        builder.swap
        builder.channel_ret

        builder.set_label(method_done_label)
        builder.local_get("self")
        builder.push(name)
        builder.channel_new(method_label)
        builder.message_new(:define_method, 2)
        builder.channel_call
        builder.pop
      end

      def self.transform_return(builder, val)
        builder.local_get("return")
        transform(builder, val)
        builder.channel_ret
      end

      def self.transform_class(builder, name, superclass, body)
        label_prefix = "Class:#{name}"
        body_label = builder.make_label(label_prefix + ".body")
        done_label = builder.make_label(label_prefix + ".done")

        # See if it's already there
        builder.channel_special(:Object)
        builder.push(name)
        builder.message_new(:const_get, 1)
        builder.channel_call
        builder.pop
        builder.jmp_if(done_label)

        # If it's not, make a new class and set it.
        builder.channel_special(:Object)
        builder.push(name)
        
        builder.channel_special(:Class)
        transform(builder, superclass)
        builder.push(name)
        builder.message_new(:new, 2)
        builder.channel_call
        builder.pop

        builder.message_new(:const_set, 2)
        builder.channel_call
        builder.pop
        builder.jmp(done_label)

        builder.set_label(body_label)
        builder.local_clean_scope
        builder.local_set("return")
        builder.message_unpack(1, 0, 0)
        builder.pop
        builder.local_set("self")
        transform(builder, body)
        builder.local_get("return")
        builder.swap
        builder.channel_ret

        builder.set_label(done_label)

        builder.dup_top
        builder.push(:__body__)
        builder.channel_new(body_label)
        builder.message_new(:define_singleton_method, 2)
        builder.channel_call
        builder.pop
        builder.pop

        builder.message_new(:__body__, 0)
        builder.channel_call
        builder.pop
      end

      def self.transform_cdecl(builder, name, val)
        # TODO: Make this look up in full proper lexical scope.
        # Currently just assumes Object is the static lexical scope
        # at all times.
        builder.channel_special(:Object)
        builder.push(name)
        transform(builder, val)
        builder.message_new(:const_set, 2)
        builder.channel_call
        builder.pop
      end
      def self.transform_const(builder, name)
        # TODO: Make this look up in full proper lexical scope.
        # Currently just assumes Object is the static lexical scope
        # at all times.
        builder.channel_special(:Object)
        builder.push(name)
        builder.message_new(:const_get, 1)
        builder.channel_call
        builder.pop
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