module Channel9
  module Ruby
    class Compiler
      attr :builder
      def initialize(builder)
        @builder = builder
        @state = {}
      end

      def with_state(hash)
        begin
          old = @state.dup
          @state.merge!(hash)
          yield
        ensure
          @state = old
        end
      end

      def transform_self()
        builder.local_get("self")
      end

      def transform_lit(literal)
        builder.push literal
      end
      def transform_str(str)
        builder.push str.intern # TODO: make this build a ruby string class instead
      end
      def transform_evstr(ev)
        transform(ev)
      end
      def transform_dstr(initial, *strings)
        strings.reverse.each do |str|
          transform(str)
        end
        if (initial.length > 0)
          transform_str(initial)
          builder.string_new(:to_s, 1 + strings.length)
        else
          builder.string_new(:to_s, strings.length)
        end
      end
      def transform_nil()
        builder.push nil.to_c9
      end
      def transform_true()
        builder.push true.to_c9
      end
      def transform_false()
        builder.push false.to_c9
      end

      def transform_while(condition, body, unk)
        begin_label = builder.make_label("while.begin")
        done_label = builder.make_label("while.done")

        builder.set_label(begin_label)
        transform(condition)
        builder.jmp_if_not(done_label)

        transform(body)
        builder.pop

        builder.jmp(begin_label)
        builder.set_label(done_label)
        builder.push(nil.to_c9)
      end

      def transform_if(cond, truthy, falsy)
        falsy_label = builder.make_label("if.falsy")
        done_label = builder.make_label("if.done")

        transform(cond)
        builder.jmp_if_not(falsy_label)

        transform(truthy)
        builder.jmp(done_label)

        builder.set_label(falsy_label)
        if (falsy)
          transform(falsy)
        else
          builder.push(nil)
        end

        builder.set_label(done_label)
      end

      def transform_args(*args)
        # TODO: Support splats and such.
        builder.message_unpack(args.count + 1, 0, 0)
        builder.pop # message stays on stack for further manipulation, we're done with it.
        args.reverse.each do |arg|
          builder.local_set(arg)
        end
        builder.local_set("self")
      end

      def transform_scope(block = nil)
        if (block.nil?)
          transform_nil()
        else
          transform(block)
        end
      end

      def transform_defn(name, args, code)
        label_prefix = "method:#{name}"
        method_label = builder.make_label(label_prefix + ".body")
        method_lret_label = builder.make_label(label_prefix + ".long_return")
        method_lret_pass = builder.make_label(label_prefix + ".long_return_pass")
        method_done_label = builder.make_label(label_prefix + ".done")

        builder.jmp(method_done_label)
        builder.set_label(method_label)
        builder.local_clean_scope
        builder.local_set("return")
        builder.channel_special(:unwinder)
        builder.channel_new(method_lret_label)
        builder.dup_top
        builder.local_set("long_return")
        builder.message_new(:set, 0, 1)
        builder.channel_call
        builder.pop
        builder.local_set("long_return_next")
        builder.message_sys_unpack(1)
        transform(args)
        builder.local_set("yield")
        transform(code)

        builder.channel_special(:unwinder)
        builder.local_get("long_return_next")
        builder.message_new(:set, 0, 1)
        builder.channel_call
        builder.pop
        builder.pop

        builder.local_get("return")
        builder.swap
        builder.channel_ret

        # TODO: Only generate a long return when the method
        # body actually has ensure blocks or a return
        # from within a block.
        builder.set_label(method_lret_label)
        # clear the unwinder.
        builder.channel_special(:unwinder)
        builder.local_get("long_return_next")
        builder.message_new(:set, 0, 1)
        builder.channel_call
        builder.pop
        builder.pop
        # stack is SP -> ret -> unwind_message
        # we want to see if the unwind_message is 
        # our return message. If so, we want to return from
        # this method. Otherwise, just move on to the next
        # unwind handler.
        builder.pop # -> unwind_message
        builder.message_name # -> name -> um
        builder.is(:long_return) # -> is -> um
        builder.jmp_if_not(method_lret_pass) # -> um
        builder.dup # -> um -> um
        builder.message_unpack(1, 0, 0) # -> um -> return_chan
        builder.swap # -> return_chan -> um
        builder.local_get("return") # -> lvar_return -> return_chan -> um
        builder.is_eq # -> is -> um
        builder.jmp_if_not(method_lret_pass) # -> um
        builder.message_unpack(2, 0, 0) # -> um -> ret_val -> return_chan
        builder.pop # -> ret_val -> return_chan
        builder.channel_ret

        builder.set_label(method_lret_pass) # (from jmps above) -> um
        builder.local_get("long_return_next") # -> lrn -> um
        builder.swap # -> um -> lrn
        builder.channel_ret

        builder.set_label(method_done_label)
        builder.local_get("self")
        builder.push(name)
        builder.channel_new(method_label)
        builder.message_new(:define_method, 0, 2)
        builder.channel_call
        builder.pop
      end

      def transform_yield(*args)
        builder.local_get("yield")

        args.each do |arg|
          transform(arg)
        end
        builder.message_new(:call, 0, args.length)
        builder.channel_call
        builder.pop
      end

      def transform_return(val)
        if (@state[:ensure] || @state[:block])
          builder.channel_special(:unwinder)
          builder.message_new(:get,0,0)
          builder.channel_call
          builder.pop

          builder.local_get("return")
          transform(val)
          builder.message_new(:long_return, 0, 2)

          builder.channel_ret
        else
          builder.channel_special(:unwinder)
          builder.local_get("long_return_next")
          builder.message_new(:set, 0, 1)
          builder.channel_call
          builder.local_get("return")
          builder.pop
          builder.pop
          transform(val)
          builder.channel_ret
        end
      end

      def transform_class(name, superclass, body)
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
          transform(superclass)
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
        transform(body)
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

      def transform_module(name, body)
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
        transform(body)
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

      def transform_cdecl(name, val)
        # TODO: Make this look up in full proper lexical scope.
        # Currently just assumes Object is the static lexical scope
        # at all times.
        builder.channel_special(:Object)
        builder.push(name)
        transform(val)
        builder.message_new(:const_set, 0, 2)
        builder.channel_call
        builder.pop
      end
      def transform_const(name)
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
      def transform_lasgn(name, val = nil)
        transform(val) if !val.nil?
        builder.dup_top
        builder.local_set(name)
      end
      def transform_lvar(name)
        builder.local_get(name)
      end

      def transform_gasgn(name, val = nil)
        if (val.nil?)
          builder.channel_special(:Object)
          builder.swap
          builder.push(name)
          builder.swap
        else
          builder.channel_special(:Object)
          builder.push(name)
          transform(val)
        end
        builder.message_new(:global_set, 0, 2)
        builder.channel_call
        builder.pop
      end
      def transform_gvar(name)
        builder.channel_special(:Object)
        builder.push(name)
        builder.message_new(:global_get, 0, 1)
        builder.channel_call
        builder.pop
      end

      def transform_iasgn(name, val = nil)
        if (val.nil?)
          builder.local_get("self")
          builder.swap
          builder.push(name)
          builder.swap
        else
          builder.local_get("self")
          builder.push(name)
          transform(val)
        end
        builder.message_new(:instance_variable_set, 0, 2)
        builder.channel_call
        builder.pop
      end
      def transform_ivar(name)
        builder.local_get("self")
        builder.push(name)
        builder.message_new(:instance_variable_get, 0, 1)
        builder.channel_call
        builder.pop
      end

      def transform_block(*lines)
        count = lines.length
        lines.each_with_index do |line, idx|
          transform(line)
          builder.pop if (count != idx + 1)
        end
      end

      def transform_call(target, method, arglist, has_iter = false)
        if (target.nil?)
          transform_self()
        else
          transform(target)
        end

        if (has_iter)
          # If we were called with has_iter == true, an iterator
          # will be sitting on the stack, and we want to swap it in
          # to first sysarg position.
          builder.swap
        end

        _, *arglist = arglist
        arglist.each do |arg|
          transform(arg)
        end

        builder.message_new(method.to_c9, has_iter ? 1 : 0, arglist.length)
        builder.channel_call
        builder.pop
      end

      # The sexp for this is weird. It embeds the call into
      # the iterator, so we build the iterator and then push it
      # onto the stack, then flag the upcoming call sexp so that it
      # swaps it in to the correct place.
      def transform_iter(call, args, block = nil)
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
          transform(args) # comes in as an lasgn or masgn
          builder.pop
        end

        if (block.nil?)
          transform_nil()
        else
          with_state(:block => true) do
            transform(block)
          end
        end

        builder.local_get(label_prefix + ".ret")
        builder.swap
        builder.channel_ret

        builder.set_label(done_label)

        builder.channel_new(body_label)

        transform(call)
      end

      def transform_ensure(body, ens)
        ens_label = builder.make_label("ensure")
        done_label = builder.make_label("ensure.done")

        builder.channel_special(:unwinder)
        builder.channel_new(ens_label)
        builder.message_new(:set, 0, 1)
        builder.channel_call
        builder.pop
        builder.local_set(ens_label + ".next")

        with_state(:ensure => ens_label) do
          transform(body)
        end
        # if the body executes correctly, we push
        # nil onto the stack so that the ensure 'channel'
        # picks that up as the return path. If it
        # does, when it goes to return, it will just
        # jmp to the done label rather than calling
        # the next handler.
        builder.push(nil.to_c9)

        builder.set_label(ens_label)
        # clear the unwinder
        builder.channel_special(:unwinder)
        builder.local_get("#{ens_label}.next")
        builder.message_new(:set, 0, 1)
        builder.channel_call
        builder.pop
        builder.pop

        # run the ensure block
        transform(ens)
        builder.pop

        # if we came here via a call (non-nil return path),
        # pass on to the next unwind handler rather than
        # leaving by the done label.
        builder.jmp_if_not(done_label)
        builder.local_get("#{ens_label}.next")
        builder.swap
        builder.channel_ret

        builder.set_label(done_label)
      end

      def transform_file(body)
        builder.local_set("return")
        builder.local_set("self")
        if (!body.nil?)
          transform(body)
        else
          transform_nil
        end
        builder.local_get("return")
        builder.swap
        builder.channel_ret
      end

      def transform(tree)
        name, *info = tree
        send(:"transform_#{name}", *info)
      end
    end
  end
end