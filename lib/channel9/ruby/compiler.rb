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
      def self.transform_nil(builder)
        builder.push nil.to_c9
      end
      def self.transform_true(builder)
        builder.push true.to_c9
      end
      def self.transform_false(builder)
        builder.push false.to_c9
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
        builder.push(nil.to_c9)
      end

      def self.transform_if(builder, cond, truthy, falsy)
        falsy_label = builder.make_label("if.falsy")
        done_label = builder.make_label("if.done")

        transform(builder, cond)
        builder.jmp_if_not(falsy_label)

        transform(builder, truthy)
        builder.jmp(done_label)

        builder.set_label(falsy_label)
        if (falsy)
          transform(builder, falsy)
        else
          builder.push(nil)
        end

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
        builder.message_sys_unpack(1)
        transform(builder, args)
        builder.local_set("yield")
        transform(builder, code)

        builder.local_get("return")
        builder.swap
        builder.channel_ret

        builder.set_label(method_done_label)
        builder.local_get("self")
        builder.push(name)
        builder.channel_new(method_label)
        builder.message_new(:define_method, 0, 2)
        builder.channel_call
        builder.pop
      end

      def self.transform_yield(builder, *args)
        builder.local_get("yield")

        args.each do |arg|
          transform(builder, arg)
        end
        builder.message_new(:call, 0, args.length)
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
        builder.message_new(:const_get, 0, 1)
        builder.channel_call
        builder.pop
        builder.dup_top
        builder.jmp_if(done_label)

        # If it's not, make a new class and set it.
        builder.pop
        builder.channel_special(:Object)
        builder.push(name)
        
        builder.channel_special(:Class)
        if (superclass.nil?)
          builder.channel_special(:Object)
        else
          transform(builder, superclass)
        end
        builder.push(name)
        builder.message_new(:new, 0, 2)
        builder.channel_call
        builder.pop

        builder.message_new(:const_set, 0, 2)
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
        builder.message_new(:define_singleton_method, 0, 2)
        builder.channel_call
        builder.pop
        builder.pop

        builder.message_new(:__body__, 0, 0)
        builder.channel_call
        builder.pop
      end

      def self.transform_module(builder, name, body)
        label_prefix = "Module:#{name}"
        body_label = builder.make_label(label_prefix + ".body")
        done_label = builder.make_label(label_prefix + ".done")

        # See if it's already there
        builder.channel_special(:Object)
        builder.push(name)
        builder.message_new(:const_get, 0, 1)
        builder.channel_call
        builder.pop
        builder.dup_top
        builder.jmp_if(done_label)

        # If it's not, make a new module and set it.
        builder.pop
        builder.channel_special(:Object)
        builder.push(name)
        
        builder.channel_special(:Module)
        builder.push(name)
        builder.message_new(:new, 0, 1)
        builder.channel_call
        builder.pop

        builder.message_new(:const_set, 0, 2)
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
        builder.message_new(:define_singleton_method, 0, 2)
        builder.channel_call
        builder.pop
        builder.pop

        builder.message_new(:__body__, 0, 0)
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
        builder.message_new(:const_set, 0, 2)
        builder.channel_call
        builder.pop
      end
      def self.transform_const(builder, name)
        # TODO: Make this look up in full proper lexical scope.
        # Currently just assumes Object is the static lexical scope
        # at all times.
        builder.channel_special(:Object)
        builder.push(name)
        builder.message_new(:const_get, 0, 1)
        builder.channel_call
        builder.pop
      end

      # If given a nil value, assumes the rhs is
      # already on the stack.
      def self.transform_lasgn(builder, name, val = nil)
        transform(builder, val) if !val.nil?
        builder.dup_top
        builder.local_set(name)
      end
      def self.transform_lvar(builder, name)
        builder.local_get(name)
      end

      def self.transform_gasgn(builder, name, val = nil)
        if (val.nil?)
          builder.channel_special(:Object)
          builder.swap
          builder.push(name)
          builder.swap
        else
          builder.channel_special(:Object)
          builder.push(name)
          transform(builder, val) if !val.nil?
        end
        builder.message_new(:global_set, 0, 2)
        builder.channel_call
        builder.pop
      end
      def self.transform_gvar(builder, name)
        builder.channel_special(:Object)
        builder.push(name)
        builder.message_new(:global_get, 0, 1)
        builder.channel_call
        builder.pop
      end

      def self.transform_block(builder, *lines)
        count = lines.length
        lines.each_with_index do |line, idx|
          transform(builder, line)
          builder.pop if (count != idx + 1)
        end
      end

      def self.transform_call(builder, target, method, arglist, has_iter = false)
        if (target.nil?)
          transform_self(builder)
        else
          transform(builder, target)
        end

        if (has_iter)
          # If we were called with has_iter == true, an iterator
          # will be sitting on the stack, and we want to swap it in
          # to first sysarg position.
          builder.swap
        end

        _, *arglist = arglist
        arglist.each do |arg|
          transform(builder, arg)
        end

        builder.message_new(method.to_c9, has_iter ? 1 : 0, arglist.length)
        builder.channel_call
        builder.pop
      end

      # The sexp for this is weird. It embeds the call into
      # the iterator, so we build the iterator and then push it
      # onto the stack, then flag the upcoming call sexp so that it
      # swaps it in to the correct place.
      def self.transform_iter(builder, call, args, block = nil)
        call = call.dup
        call << true

        label_prefix = "Iter:#{call[2]}"
        body_label = builder.make_label(label_prefix + ".body")
        done_label = builder.make_label(label_prefix + ".done")

        builder.jmp(done_label)

        builder.set_label(body_label)
        builder.local_set(label_prefix + ".ret")
        if (args.nil?)
          # no args, pop the message off the stack.
          builder.pop
        else
          if (args[0] == :lasgn || args[0] == :gasgn)
            # Ruby's behaviour on a single arg block is ugly.
            # If it takes one argument, but is given multiple,
            # it's as if it were a single arg splat. Otherwise,
            # it's like a normal method invocation.
            builder.message_count
            builder.is_not(1.to_c9)
            builder.jmp_if(label_prefix + ".splatify")
            builder.message_unpack(1, 0, 0)
            builder.jmp(label_prefix + ".done_unpack")
            builder.set_label(label_prefix + ".splatify")
            builder.message_unpack(0, 1, 0)
            builder.set_label(label_prefix + ".done_unpack")
          else
            builder.message_unpack(0, 1, 0) # splat it all for the masgn
          end
          builder.pop
          transform(builder, args) # comes in as an lasgn or masgn
          builder.pop
        end

        if (block.nil?)
          transform_nil(builder)
        else
          transform(builder, block)
        end

        builder.local_get(label_prefix + ".ret")
        builder.swap
        builder.channel_ret

        builder.set_label(done_label)

        builder.channel_new(body_label)

        transform(builder, call)
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