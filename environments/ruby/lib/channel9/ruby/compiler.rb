require 'set'
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
          @state = @state.merge(hash)
          yield
        ensure
          @state = old
        end
      end

      def with_new_vtable(&block)
        with_state(:vtable => [Set.new], &block)
      end
      def with_linked_vtable(&block)
        parent_vtables = @state[:vtable] || []
        with_state(:vtable => [Set.new, *parent_vtables], &block)
      end
      def find_lexical_depth(name, add = false)
        @state[:vtable].each_with_index do |tbl, idx|
          if tbl.include?(name.to_sym)
            return idx
          end
        end
        if (add)
          @state[:vtable].first << name.to_sym
        end
        return 0 # default to current scope.
      end

      # Pushes the constant self onto the stack,
      # using the const-self frame variable.
      # If name is passed in, will also
      # evaluate any :const/:colon2/:colon3
      # trees and return the rightmost name component
      # (just returning name if name is a bare symbol
      # or a :const sexp)
      def const_self(name = nil)
        if (name)
          if (name.is_a? Symbol)
            const_self # no name
            return name
          elsif (name.first == :const)
            const_self
            return name[1]
          elsif (name.first == :colon3) # global scoped
            builder.channel_special(:Object)
            return name[1]
          elsif (name.first == :colon2) # scoped
            transform(name[1])
            return name[2]
          else
            raise_error NotImplementedError, "Unknown constant type #{name.first}"
          end
        else
          builder.frame_get("const-self")
          builder.tuple_unpack(1, 0, 0)
          builder.swap
          builder.pop # get rid of the tuple.
        end
      end

      def raise_error(type, desc)
        transform_self
        transform_colon3(type.to_sym)
        builder.push(desc.to_sym)
        builder.message_new(:raise, 0, 2)
        builder.channel_ret
      end

      def transform_self()
        builder.frame_get("self")
      end

      def transform_lit(literal)
        case literal
        when Range
          if (literal.exclude_end?)
            transform_dot3([:lit, literal.first], [:lit, literal.last])
          else
            transform_dot2([:lit, literal.first], [:lit, literal.last])
          end
        when Regexp
          builder.channel_special(:Regexp)
          builder.push(literal.to_s)
          builder.message_new(:new, 0, 1)
          builder.channel_call
          builder.pop
        else
          builder.push literal
        end
      end
      def transform_dot2(first, last)
        builder.channel_special(:Range)
        transform(first)
        transform(last)
        builder.push(false)
        builder.message_new(:new, 0, 3)
        builder.channel_call
        builder.pop
      end
      def transform_dot3(first, last)
        builder.channel_special(:Range)
        transform(first)
        transform(last)
        builder.push(true)
        builder.message_new(:new, 0, 3)
        builder.channel_call
        builder.pop
      end

      def transform_str(str)
        builder.channel_special(:String)
        builder.push(str)
        builder.message_new(:new, 0, 1)
        builder.channel_call
        builder.pop
      end
      def transform_xstr(str)
        transform_self
        builder.push(str)
        builder.message_new(:system, 0, 1)
        builder.channel_call
        builder.pop
      end
      def transform_dxstr(initial, *strings)
        transform_self
        transform_dsym(initial, *strings)
        builder.message_new(:system, 0, 1)
        builder.channel_call
        builder.pop
      end
      def transform_evstr(ev)
        transform(ev)
      end
      def transform_dsym(initial, *strings)
        strings.reverse.each do |str|
          transform(str)
          builder.string_coerce(:to_s_prim)
          builder.pop
        end
        if (initial.length > 0)
          transform_str(initial)
          builder.string_coerce(:to_s_prim)
          builder.pop
          builder.string_new(1 + strings.length)
        else
          builder.string_new(strings.length)
        end
      end
      def transform_dstr(initial, *strings)
        builder.channel_special(:String)
        transform_dsym(initial, *strings)
        builder.message_new(:new, 0, 1)
        builder.channel_call
        builder.pop
      end
      def transform_dregx(initial, *strings)
        flags = if (strings.last.is_a? Fixnum)
          strings.pop
        end
        builder.channel_special(:Regexp)
        transform_dsym(initial, *strings)
        builder.message_new(:new, 0, 1)
        builder.channel_call
        builder.pop
      end
      def transform_dregx_once(initial, *strings)
        flags = if (strings.last.is_a? Fixnum)
          strings.pop
        end
        opt = strings.pop
        builder.channel_special(:Regexp)
        transform_dsym(initial, *strings)
        builder.message_new(:new, 0, 1)
        builder.channel_call
        builder.pop
      end
      def transform_hash(*items)
        builder.channel_special(:Hash)
        items.reverse.each do |item|
          transform(item)
        end
        builder.tuple_new(items.length)
        builder.message_new(:new_from_tuple, 0, 1)
        builder.channel_call
        builder.pop
      end

      def transform_splat(from)
        transform(from)
        builder.message_new(:to_tuple_prim, 0, 0)
        builder.channel_call
        builder.pop
        builder.tuple_splat
      end
      def transform_array(*items)
        splat = nil
        if (items.last && items.last[0] == :splat)
          items = items.dup
          splat = items.pop
        end

        builder.channel_special(:Array)
        items.reverse.each do |item|
          transform(item)
        end
        builder.tuple_new(items.length)
        transform(splat) if splat
        builder.message_new(:new_from_tuple, 0, 1)
        builder.channel_call
        builder.pop
      end
      def transform_nil()
        builder.push nil
      end
      def transform_true()
        builder.push true
      end
      def transform_false()
        builder.push false
      end

      def transform_while(condition, body, unk)
        begin_label = builder.make_label("while.begin")
        redo_label = builder.make_label("while.redo")
        done_label = builder.make_label("while.done")

        builder.set_label(begin_label)
        transform(condition)
        builder.jmp_if_not(done_label)

        linfo = {
          :type => :while,
          :beg_label => begin_label,
          :end_label => done_label,
          :retry_label => begin_label,
          :next_label => begin_label,
          :redo_label => redo_label,
        }
        with_state(:loop => linfo) do
          builder.set_label(redo_label)
          transform(body)
        end
        builder.pop

        builder.jmp(begin_label)
        builder.set_label(done_label)
        builder.push(nil)
      end
      def transform_until(condition, body, unk)
        begin_label = builder.make_label("until.begin")
        redo_label = builder.make_label("until.redo")
        done_label = builder.make_label("until.done")

        builder.set_label(begin_label)
        transform(condition)
        builder.jmp_if(done_label)

        linfo = {
          :type => :while,
          :beg_label => begin_label,
          :end_label => done_label,
          :retry_label => begin_label,
          :next_label => begin_label,
          :redo_label => redo_label,
        }
        with_state(:loop => linfo) do
          builder.set_label(redo_label)
          transform(body)
        end
        builder.pop

        builder.jmp(begin_label)
        builder.set_label(done_label)
        builder.push(nil)
      end

      def transform_flip2(l, r)
        raise_error :NotImplementedError, "(i == N)..(i == M) syntax not supported. Do you really use this?"
      end
      def transform_flip3(l, r)
        raise_error :NotImplementedError, "(i == N)...(i == M) syntax not supported. Do you really use this?"
      end

      def transform_if(cond, truthy, falsy)
        falsy_label = builder.make_label("if.falsy")
        done_label = builder.make_label("if.done")

        transform(cond)
        builder.jmp_if_not(falsy_label)

        if (truthy.nil?)
          transform_nil
        else
          transform(truthy)
        end
        builder.jmp(done_label)

        builder.set_label(falsy_label)
        if (falsy)
          transform(falsy)
        else
          builder.push(nil)
        end

        builder.set_label(done_label)
      end

      def transform_not(val)
        # TODO? it seems a little expensive to test
        # this with a jmp, but current instruction set
        # doesn't provide an is_truthy kind of thing. I'm
        # not sure if it should be added, though.
        truthy_label = builder.make_label("not.truthy")
        done_label = builder.make_label("not.done")
        transform(val)
        builder.jmp_if_not(truthy_label)
        builder.push(false)
        builder.jmp(done_label)
        builder.set_label(truthy_label)
        builder.push(true)
        builder.set_label(done_label)
      end

      def transform_or(left, right)
        done_label = builder.make_label("or.done")

        transform(left)
        builder.dup_top
        builder.jmp_if(done_label)
        builder.pop
        transform(right)
        builder.set_label(done_label)
      end
      def transform_and(left, right)
        done_label = builder.make_label("and.done")

        transform(left)
        builder.dup_top
        builder.jmp_if_not(done_label)
        builder.pop
        transform(right)
        builder.set_label(done_label)
      end

      def transform_match3(val, cmp)
        transform(val)
        transform(cmp)
        builder.message_new(:=~, 0, 1)
        builder.channel_call
        builder.pop
      end
      alias_method :transform_match2, :transform_match3

      def transform_op_asgn_or(var_if, asgn)
        done_label = builder.make_label("asgn_or.done")
        transform(var_if)
        builder.dup_top
        builder.jmp_if(done_label)
        builder.pop # get rid of the falsy left behind.
        transform(asgn)
        builder.set_label(done_label)
      end
      def transform_op_asgn_and(var_if, asgn)
        done_label = builder.make_label("asgn_and.done")
        transform(var_if)
        builder.dup_top
        builder.jmp_if_not(done_label)
        builder.pop # get rid of the falsy left behind.
        transform(asgn)
        builder.set_label(done_label)
      end

      def transform_op_asgn1(var_if, args, op, val)
        case op
        when :'||', :'&&'
          done_label = builder.make_label("asgn1.done")
          transform_call(var_if, :[], args)
          builder.dup_top
          case op
          when :'||'
            builder.jmp_if(done_label)
          when :'&&'
            builder.jmp_if_not(done_label)
          end
          builder.pop
          asgn_args = args + s(val)
          transform_call(var_if, :[]=, asgn_args)
          builder.set_label(done_label)
        else
          transform(var_if) # -> var
          builder.dup_top # -> var -> var
          _, *args = args
          args.reverse.each do |arg|
            transform(arg)
          end # -> args... -> var -> var
          builder.tuple_new(args.length) # -> args -> var -> var
          builder.dup_top # -> args -> args -> var -> var
          builder.frame_set("asgn1.tuple") # -> args -> var -> var
          builder.message_new(:[], 0, 0) # -> []msg -> args -> var -> var
          builder.swap # -> args -> msg -> var -> var
          builder.message_splat # -> msg -> var -> var
          builder.channel_call
          builder.pop # -> orig -> var

          transform(val) # -> opvar -> orig -> var
          builder.swap # -> orig -> opvar -> var
          builder.message_new(op, 0, 1) # -> msg -> opvar -> var
          builder.channel_call
          builder.pop # -> opres -> var

          builder.tuple_new(1) # -> restuple -> var
          builder.message_new(:[]=, 0, 0) # -> msg -> restuple -> var
          builder.frame_get("asgn1.tuple") # -> args -> msg -> restuple -> var
          builder.message_splat # -> msg -> restuple -> var
          builder.swap # -> restuple -> msg -> var
          builder.message_splat # -> msg -> var
          builder.channel_call
          builder.pop # -> res
        end
      end

      def transform_op_asgn2(var_if, attrib_write, op, val)
        attrib_read = attrib_write.to_s.gsub(%r{=$}, '').to_sym
        args = s(:arglist)
        case op
        when :'||', :'&&'
          done_label = builder.make_label("asgn1.done")
          transform_call(var_if, attrib_read, args)
          builder.dup_top
          case op
          when :'||'
            builder.jmp_if(done_label)
          when :'&&'
            builder.jmp_if_not(done_label)
          end
          builder.pop
          asgn_args = args + s(val)
          transform_call(var_if, attrib_write, asgn_args)
          builder.set_label(done_label)
        else
          transform(var_if) # -> var
          builder.dup_top # -> var -> var
          _, *args = args
          args.reverse.each do |arg|
            transform(arg)
          end # -> args... -> var -> var
          builder.tuple_new(args.length) # -> args -> var -> var
          builder.dup_top # -> args -> args -> var -> var
          builder.frame_set("asgn1.tuple") # -> args -> var -> var
          builder.message_new(attrib_read, 0, 0) # -> []msg -> args -> var -> var
          builder.swap # -> args -> msg -> var -> var
          builder.message_splat # -> msg -> var -> var
          builder.channel_call
          builder.pop # -> orig -> var -> var

          transform(val) # -> opvar -> orig -> var -> var
          builder.swap # -> orig -> opvar -> var -> var
          builder.message_new(op, 0, 1) # -> msg -> var -> var
          builder.channel_call
          builder.pop # -> opres -> var

          builder.tuple_new(1) # -> restuple -> var
          builder.message_new(attrib_write, 0, 0) # -> msg -> restuple -> var
          builder.frame_get("asgn1.tuple") # -> args -> msg -> restuple -> var
          builder.message_splat # -> msg -> restuple -> var
          builder.swap # -> restuple -> msg -> var
          builder.message_splat # -> msg -> var
          builder.channel_call
          builder.pop # -> res
        end
      end

      def transform_when(comparisons, body)
        found_label = builder.make_label("when.found")
        next_label = builder.make_label("when.next")
        done_label = @state[:case_done]

        if (comparisons.first == :array && comparisons[1].first == :when)
          # for some reason, a single splat as the argument to when puts
          # an embedded when as the when. This untangles that mess. Not sure why
          # it's like that, though.
          comparisons = comparisons.dup
          comparisons[1] = comparisons[1][1]
        end
        if (comparisons.first == :array)
          comparisons = comparisons.dup
          comparisons.shift
        else
          comparisons = [comparisons]
        end

        comparisons.each do |cmp|
          if (@state[:if_case])
            transform(cmp)
            builder.jmp_if(found_label)
            builder.jmp(next_label)
          else
            builder.dup_top # value from enclosing case.
            transform(cmp)
            builder.swap
            builder.message_new(:===, 0, 1)
            builder.channel_call
            builder.pop
            builder.jmp_if(found_label)
            builder.jmp(next_label)
          end
        end
        builder.set_label(found_label)
        builder.pop if (!@state[:if_case])# value not needed anymore
        if (body.nil?)
          transform_nil
        else
          transform(body)
        end
        builder.jmp(done_label)
        builder.set_label(next_label)
      end

      def transform_case(value, *cases)
        else_case = cases.pop
        done_label = builder.make_label("case.done")

        if (value)
          transform(value)
        end

        with_state(:case_done => done_label, :if_case => value.nil?) do
          cases.each do |case_i|
            transform(case_i)
          end
        end

        if (!value.nil?)
          builder.pop # value not needed anymore.
        end
        if (else_case.nil?)
          builder.push(nil)
        else
          transform(else_case)
        end

        builder.set_label(done_label)
      end

      def transform_args(*args)
        defargs = []
        splatarg = nil
        if (args.length > 0)
          if (!args.last.is_a?(Symbol))
            defargs = args.pop.dup
            defargs.shift # get rid of the :block lead
          end
          if (match = args.last.to_s.match(%r{^\&(.+)}))
            blockarg = match[1].to_sym
            args.pop
          end
          if (match = args.last.to_s.match(%r{^\*(.+)}))
            splatarg = match[1].to_sym
            args.pop
          end
        end

        if (args.length)
          if (defargs.length == 0)
            builder.message_unpack(args.length, splatarg ? 1 : 0, 0)
            args.each do |arg|
              builder.lexical_set(find_lexical_depth(arg, true), arg)
            end
          else
            must_have = args.length - defargs.length
            argdone_label = builder.make_label("args.done")
            defarg_labels = (0...defargs.length).collect {|i| builder.make_label("args.default.#{i}") }

            builder.message_count
            builder.frame_set("arg.count")
            builder.message_unpack(args.length, splatarg ? 1 : 0, 0)
            i = 0
            must_have.times do
              builder.lexical_set(find_lexical_depth(args[i], true), args[i])
              i += 1
            end

            while (i < args.length)
              builder.frame_get("arg.count")
              builder.is(i)
              builder.jmp_if(defarg_labels[i-must_have])
              builder.lexical_set(find_lexical_depth(args[i], true), args[i])
              i += 1
            end
            builder.jmp(argdone_label)

            defarg_labels.each do |defarg_label|
              builder.set_label(defarg_label)
              builder.pop # undef padding value
              transform(defargs.shift)
              builder.pop # result of assignment
            end

            builder.set_label(argdone_label)
          end
          if (splatarg)
            builder.channel_special(:Array)
            builder.swap
            builder.message_new(:new_from_tuple, 0, 1)
            builder.channel_call
            builder.pop
            builder.lexical_set(find_lexical_depth(splatarg, true), splatarg)
          end
          if (blockarg)
            no_yield_label = builder.make_label("no_yield")
            builder.frame_get("yield")
            builder.dup_top
            builder.jmp_if_not(no_yield_label)
            builder.channel_special(:Proc)
            builder.swap
            builder.message_new(:new_from_prim, 0, 1)
            builder.channel_call
            builder.pop
            builder.lexical_set(find_lexical_depth(blockarg, true), blockarg)
            builder.push(nil)
            builder.set_label(no_yield_label)
            builder.pop
          end
        end
        builder.pop
      end

      def transform_scope(block = nil)
        if (block.nil?)
          transform_nil()
        else
          transform(block)
        end
      end

      def need_return_unwind(name, code)
        # to determine if the function body
        # requires a return unwind handler, we
        # compile the body just to find out if
        # there's a return inside an ensure
        # or a block. If there is, we know
        # the function will need an unwind
        # handler.
        stream = Stream.new
        builder = Builder.new(stream)
        compiler = Compiler.new(builder)
        need_info = [false]
        compiler.with_state(@state) do
          compiler.with_state(:need_long_return => need_info, :name => name, :unwinders => 0) do
            compiler.transform(code)
            return need_info[0]
          end
        end
      end

      def transform_undef(name)
        const_self
        transform(name)
        builder.message_new(:undef_method, 0, 1)
        builder.channel_call
        builder.pop
      end

      def transform_defn(name, args, code)
        transform_defs(nil, name, args, code)
      end

      def transform_defs(on, name, args, code)
        label_prefix = "method:#{name}"
        method_label = builder.make_label(label_prefix + ".body")
        method_lret_label = builder.make_label(label_prefix + ".long_return")
        method_lret_pass = builder.make_label(label_prefix + ".long_return_pass")
        method_done_label = builder.make_label(label_prefix + ".done")

        builder.jmp(method_done_label)
        builder.set_label(method_label)
        builder.lexical_clean_scope
        builder.frame_set("return")
        builder.dup_top
        builder.frame_set("message")

        if (nru = need_return_unwind(name, code))
          builder.channel_special(:unwinder)
          builder.channel_new(method_lret_label)
          builder.channel_call
          builder.pop
          builder.pop
        end

        with_new_vtable do
          builder.message_sys_unpack(3)
          builder.frame_set("self")
          builder.frame_set("super")
          builder.frame_set("yield")
          transform(args)

          with_state(:has_long_return => nru, :name => name, :unwinders => 0, :loop => nil) do
            if (code.nil?)
              transform_nil
            else
              transform(code)
            end
          end
        end

        # remove the long return handler if there is one.
        if (nru)
          builder.channel_special(:unwinder)
          builder.push(nil)
          builder.channel_call
          builder.pop
          builder.pop
        end

        builder.frame_get("return")
        builder.swap
        builder.channel_ret

        if (nru)
          builder.set_label(method_lret_label)
          # stack is SP -> ret -> unwind_message
          # we want to see if the unwind_message is
          # our return message. If so, we want to return from
          # this method. Otherwise, just move on to the next
          # unwind handler.
          builder.local_set(:unwind_return) # -> unwind_message
          builder.message_id # -> name -> um
          builder.is(Primitive::MessageID.new("long_return")) # -> is -> um
          builder.jmp_if_not(method_lret_pass) # -> um
          builder.dup # -> um -> um
          builder.message_unpack(1, 0, 0) # -> return_chan -> um
          builder.frame_get("return") # -> lvar_return -> return_chan -> um
          builder.is_eq # -> is -> um
          builder.jmp_if_not(method_lret_pass) # -> um
          builder.message_unpack(2, 0, 0) # -> return_chan -> ret_val -> um
          builder.swap # -> ret_val -> return_chan -> um
          builder.channel_ret

          builder.set_label(method_lret_pass) # (from jmps above) -> um
          builder.pop
          builder.local_get(:unwind_return)
          builder.push(false)
          builder.channel_ret
        end
        builder.set_label(method_done_label)
        if (on.nil?)
          const_self
        else
          transform(on)
        end
        if (on)
          builder.message_new(:"ruby_sys:make_singleton", 0, 0)
          builder.channel_call
          builder.pop
        end
        builder.push(Primitive::MessageID.new(name))
        builder.channel_new(method_label)
        builder.message_new(:"ruby_sys:add_method", 0, 2)
        builder.channel_call
        builder.pop
      end

      def transform_super(*args)
        label = builder.make_label("super")
        fail_label = label + ".fail"
        done_label = label + ".done"

        has_iter = false

        # get the new call and the new super
        builder.frame_get("super")
        builder.push(nil)
        builder.channel_call

        # swizzle them around so we can check if we got one.
        builder.swap
        builder.dup_top
        builder.jmp_if_not(fail_label)
        builder.swap

        builder.frame_get("self")
        builder.swap

        # add the new arguments
        if (args.last && args.last[0] == :block_pass)
          block_arg = args.pop.dup
          has_iter = true
          transform(block_arg[1])
          builder.message_new(:to_proc_prim, 0, 0)
          builder.channel_call
          builder.pop
        end

        if (first_splat = has_splat(args))
          i = 0
          while (i < first_splat)
            transform(args.shift)
            i += 1
          end
          builder.message_new(@state[:name] || :super, 2 + (has_iter ? 1 : 0), i)
          transform(args.shift[1])
          builder.message_new(:to_tuple_prim, 0, 0)
          builder.channel_call
          builder.pop
          builder.message_splat
        else
          args.each do |arg|
            transform(arg)
          end

          builder.message_new(@state[:name] || :super, 2 + (has_iter ? 1 : 0), args.length)
        end
        # TODO: the name should be dynamically derived, but for now it doesn't
        # really matter what name we're sending along. If that ever changes, this needs
        # to change.

        builder.channel_call
        builder.pop

        builder.jmp(done_label)
        builder.set_label(fail_label)

        raise_error(:NoMethodError, "No superclass method for `#{@state[:name]}'")

        builder.set_label(done_label)
      end

      def transform_zsuper
        label = builder.make_label("zsuper")
        fail_label = label + ".fail"
        done_label = label + ".done"

        # get the new call and the new super
        builder.frame_get("super")
        builder.push(nil)
        builder.channel_call

        # swizzle them around so we can check if we got one.
        builder.swap
        builder.dup_top
        builder.jmp_if_not(fail_label)
        builder.swap

        builder.frame_get("self")
        builder.swap

        builder.frame_get("yield")

        # see super about issues with the name.
        builder.message_new(@state[:name] || :super, 3, 0)

        builder.frame_get("message")
        builder.message_unpack(0, 3, 0)
        builder.swap
        builder.pop

        builder.message_splat

        builder.channel_call
        builder.pop

        builder.jmp(done_label)
        builder.set_label(fail_label)

        raise_error(:NoMethodError, "No superclass method for `#{@state[:name]}'")

        builder.set_label(done_label)
      end

      def transform_yield(*args)
        builder.frame_get("yield")

        sofar = 0
        message_created = false
        args.each do |arg|
          if (arg[0] != :splat)
            transform(arg)
            sofar += 1
          else
            builder.message_new(:call, 0, sofar)
            transform(arg[1])
            builder.message_new(:to_tuple_prim, 0, 0)
            builder.channel_call
            builder.pop
            builder.message_splat
            message_created = true
          end
        end
        builder.message_new(:call, 0, args.length) if !message_created
        builder.channel_call
        builder.pop
      end

      def transform_svalue(splat)
        # basic effect of a single splatted value
        # in a return statement is to turn no or
        # one value into unpacking the first value
        # from the tuple, otherwise just return the tuple.
        splat_done = builder.make_label("splat.done")
        splat_to_a = builder.make_label("splat.to_a")
        transform(splat[1])
        builder.message_new(:to_tuple_prim, 0, 0)
        builder.channel_call
        builder.pop
        builder.dup_top
        builder.message_new(:length, 0, 0)
        builder.channel_call
        builder.pop
        builder.push(2)
        builder.message_new(:<, 0, 1)
        builder.channel_call
        builder.pop
        builder.jmp_if_not(splat_to_a)
        builder.tuple_unpack(1,0,0)
        builder.swap
        builder.pop
        builder.jmp(splat_done)
        builder.set_label(splat_to_a)
        builder.message_new(:to_a, 0, 0)
        builder.channel_call
        builder.pop
        builder.set_label(splat_done)
      end

      def transform_return_vals(vals)
        if (vals.any?)
          if (vals.length == 1)
            transform(vals[0])
          else
            splat = nil
            if (vals.last[0] == :svalue)
              splat = vals.pop
            end
            vals.reverse.each do |val|
              transform(val)
            end
            builder.tuple_new(vals.length)
            if (splat)
              transform(splat[1])
              builder.tuple_new(1)
              builder.tuple_splat
            end
          end
        else
          transform_nil
        end
      end

      def transform_return(*vals)
        if (@state[:block] || (@state[:unwinders] && @state[:unwinders] > 0))
          @state[:need_long_return][0] = true if @state[:need_long_return]
          builder.channel_special(:unwinder)
          builder.frame_get("return")

          transform_return_vals(vals)

          builder.message_new(:long_return, 0, 2)
          builder.channel_ret
        else
          builder.frame_get("return")

          transform_return_vals(vals)

          builder.channel_ret
        end
      end

      def transform_sclass(obj, body)
        label_prefix = "sclass"
        body_label = builder.make_label(label_prefix + ".body")
        done_label = builder.make_label(label_prefix + ".done")

        transform(obj)
        builder.message_new(:"ruby_sys:make_singleton", 0, 0)
        builder.channel_call
        builder.pop
        builder.jmp(done_label)

        builder.set_label(body_label)
        builder.lexical_clean_scope
        builder.frame_set("return")
        builder.message_sys_unpack(1)
        builder.dup_top
        builder.frame_set("self")
        builder.tuple_new(1)
        builder.frame_get("const-self")
        builder.tuple_new(1)
        builder.tuple_splat
        builder.frame_set("const-self")
        builder.pop
        if (body.nil?)
          transform_nil
        else
          with_new_vtable do
            transform(body)
          end
        end
        builder.frame_get("return")
        builder.swap
        builder.channel_ret

        builder.set_label(done_label)

        builder.channel_new(body_label)
        builder.message_new(:"ruby_sys:instance_eval", 1, 0)
        builder.channel_call
        builder.pop
      end

      def transform_class(name, superclass, body)
        label_prefix = builder.make_label("Class:#{name}")
        body_label = label_prefix + ".body"
        make_label = label_prefix + ".make"
        done_label = label_prefix + ".done"

        # See if it's already there
        bare_name = const_self(name)
        builder.dup_top
        builder.frame_set("new-const-self")
        builder.push(bare_name)
        builder.message_new(:"ruby_sys:get_constant", 0, 1)
        builder.channel_call
        builder.pop

        builder.dup_top
        builder.jmp_if_not(make_label)

        # Make sure it's a class
        builder.dup_top
        builder.channel_special(:Class)
        builder.swap
        builder.message_new(:"ruby_sys:class", 0, 0)
        builder.channel_call
        builder.pop
        builder.is_eq
        builder.jmp_if(done_label)

        # It's not a class, so error out here.
        builder.pop
        raise_error(:TypeError, "#{bare_name} is not a Class.")

        # If it's not, make a new class and set it.
        builder.set_label(make_label)
        builder.pop

        builder.frame_get("new-const-self")
        builder.push(bare_name)

        builder.channel_special(:Class)
        if (superclass.nil?)
          builder.channel_special(:Object)
        else
          transform(superclass)
        end

        builder.frame_get("new-const-self")
        builder.message_new(:"ruby_sys:scope_name", 0, 0)
        builder.channel_call
        builder.pop
        builder.push(bare_name)
        builder.message_new(:+, 0, 1)
        builder.channel_call
        builder.pop

        builder.swap
        builder.message_new(:"ruby_sys:allocate", 0, 2)
        builder.channel_call
        builder.pop

        builder.message_new(:"ruby_sys:add_constant", 0, 2)
        builder.channel_call
        builder.pop

        builder.jmp(done_label)

        builder.set_label(body_label)
        builder.lexical_clean_scope
        builder.frame_set("return")
        builder.message_sys_unpack(1)
        builder.dup_top
        builder.frame_set("self")
        builder.tuple_new(1)
        builder.frame_get("const-self")
        builder.tuple_new(1)
        builder.tuple_splat
        builder.frame_set("const-self")
        builder.pop
        if (body.nil?)
          transform_nil
        else
          with_new_vtable do
            transform(body)
          end
        end
        builder.frame_get("return")
        builder.swap
        builder.channel_ret

        builder.set_label(done_label)
        builder.push(nil)
        builder.frame_set("new-const-self")

        builder.dup_top
        builder.message_new(:"ruby_sys:make_singleton", 0, 0)
        builder.channel_call
        builder.pop
        builder.push(Primitive::MessageID.new("__body__"))
        builder.channel_new(body_label)
        builder.message_new(:"ruby_sys:add_method", 0, 2)
        builder.channel_call
        builder.pop
        builder.pop

        builder.message_new(:__body__, 0, 0)
        builder.channel_call
        builder.pop
      end

      def transform_module(name, body)
        label_prefix = builder.make_label("Module:#{name}")
        body_label = label_prefix + ".body"
        make_label = label_prefix + ".make"
        done_label = label_prefix + ".done"

        # See if it's already there
        bare_name = const_self(name)
        builder.push(bare_name)
        builder.message_new(:"ruby_sys:get_constant", 0, 1)
        builder.channel_call
        builder.pop

        builder.dup_top
        builder.jmp_if_not(make_label)

        builder.dup_top
        builder.channel_special(:Module)
        builder.swap
        builder.message_new(:"ruby_sys:class", 0, 0)
        builder.channel_call
        builder.pop
        builder.is_eq
        builder.jmp_if(done_label)

        # It's not a module, so error out here.
        builder.pop
        raise_error(:TypeError, "#{bare_name} is not a Module.")

        builder.set_label(make_label)
        # If it's not, make a new module and set it.
        builder.pop

        const_self(name)
        builder.dup_top
        builder.push(bare_name)

        builder.swap
        builder.message_new(:"ruby_sys:scope_name", 0, 0)
        builder.channel_call
        builder.pop
        builder.push(bare_name)
        builder.message_new(:+, 0, 1)
        builder.channel_call
        builder.pop

        builder.channel_special(:Module)
        builder.swap

        builder.message_new(:"ruby_sys:allocate", 0, 1)
        builder.channel_call
        builder.pop

        builder.message_new(:"ruby_sys:add_constant", 0, 2)
        builder.channel_call
        builder.pop
        builder.jmp(done_label)

        builder.set_label(body_label)
        builder.lexical_clean_scope
        builder.frame_set("return")
        builder.message_sys_unpack(1)
        builder.dup_top
        builder.frame_set("self")
        builder.tuple_new(1)
        builder.frame_get("const-self")
        builder.tuple_new(1)
        builder.tuple_splat
        builder.frame_set("const-self")
        builder.pop
        if (body.nil?)
          transform_nil
        else
          with_new_vtable do
            transform(body)
          end
        end
        builder.frame_get("return")
        builder.swap
        builder.channel_ret

        builder.set_label(done_label)

        builder.dup_top
        builder.message_new(:"ruby_sys:make_singleton", 0, 0)
        builder.channel_call
        builder.pop
        builder.push(Primitive::MessageID.new('__body__'))
        builder.channel_new(body_label)
        builder.message_new(:"ruby_sys:add_method", 0, 2)
        builder.channel_call
        builder.pop
        builder.pop

        builder.message_new(:__body__, 0, 0)
        builder.channel_call
        builder.pop
      end

      def transform_alias(first, second)
        builder.frame_get("self")
        builder.push(Primitive::MessageID.new(first[1]))
        builder.push(Primitive::MessageID.new(second[1]))
        builder.message_new(:"ruby_sys:alias_method", 0, 2)
        builder.channel_call
        builder.pop
      end

      def transform_valias(first, second)
        # TODO: Implement this properly.
        # It's for aliasing globals (alias $X $Y makes $Y accessible through $X)
        # Used in lib/English.rb.
        builder.push(nil)
      end

      def transform_cdecl(name, val = nil)
        bare_name = const_self(name)
        if (val.nil?)
          builder.swap # value is in stack before this.
        else
          transform(val)
        end
        builder.push(bare_name)
        builder.swap # contortion necessary to deal with pulling up value from stack.
        builder.message_new(:"ruby_sys:add_constant", 0, 2)
        builder.channel_call
        builder.pop
      end
      def transform_const(name)
        if (name == 'C9_CONST_SELF')
          builder.frame_get("const-self")
        else
          builder.frame_get("const-self")
          builder.tuple_unpack(2, 0, 0)
          builder.swap
          builder.push(name)
          builder.swap
          builder.message_new(:"ruby_sys:get_constant_scoped", 0, 2)
          builder.channel_call
          builder.pop
          builder.swap
          builder.pop # get rid of the const-self
        end
      end

      def transform_colon3(name)
        builder.channel_special(:Object)
        builder.push(name)
        builder.message_new(:"ruby_sys:get_constant", 0, 1)
        builder.channel_call
        builder.pop
      end

      def transform_colon2(lhs, name)
        transform(lhs)
        builder.push(name)
        builder.message_new(:"ruby_sys:get_constant", 0, 1)
        builder.channel_call
        builder.pop
      end

      def transform_to_ary(val)
        transform(val)
        if (val[0] != :array)
          builder.message_new(:to_a, 0, 0)
          builder.channel_call
          builder.pop
        end
      end

      def transform_masgn(lhs, rhs = nil)
        # lhs is always an array, get rid of its
        # prefix so we can deal with it sensibly
        lhs = lhs.dup
        lhs.shift
        left_splat = nil

        if (lhs.last[0] == :splat)
          splat = lhs.pop
        end

        if (!rhs.nil?) # rhs already on stack?
          if (rhs[0] == :splat)
            rhs = rhs[1]
            transform(rhs)
          else
            transform(rhs)
          end
        end
        builder.dup_top # so it's the result.
        # rhs should now be an array we can convert to a tuple
        # and unpack
        builder.message_new(:to_tuple_prim, 0, 0)
        builder.channel_call
        builder.pop
        builder.tuple_unpack(lhs.length, splat.nil? ? 0 : 1, 0)

        with_state(:masgn => true) do
          lhs.each do |asgn|
            transform(asgn)
            builder.pop
          end
        end
        if splat
          if (splat.length > 1)
            builder.message_new(:to_a, 0, 0)
            builder.channel_call
            builder.pop
            if (splat[1])
              transform(splat[1])
            else
              builder.pop
            end
            builder.pop # don't want the result of the individual assignments.
          else
            builder.pop # don't want anything, splat to the void!
          end
        end
        builder.pop # tuple from the tuple_unpack is still there.
      end

      # If given a nil value, assumes the rhs is
      # already on the stack.
      def transform_lasgn(name, val = nil)
        raise SyntaxError.new("Assigning to nothing? on line #{current_sexp.file}:#{current_sexp.line}") if !name
        transform(val) if !val.nil?
        builder.dup_top
        builder.lexical_set(find_lexical_depth(name, true), name)
      end
      def transform_lvar(name)
        builder.lexical_get(find_lexical_depth(name), name)
      end

      def transform_gasgn(name, val = nil)
        if (val.nil?)
          builder.channel_special(:globals)
          builder.swap
          builder.push(name)
          builder.swap
        else
          builder.channel_special(:globals)
          builder.push(name)
          transform(val)
        end
        builder.message_new(:set, 0, 2)
        builder.channel_call
        builder.pop
      end
      def transform_gvar(name)
        if (match = name.to_s.match(/^\$__c9_(.+)$/))
          builder.channel_special(match[1])
        else
          builder.channel_special(:globals)
          builder.push(name)
          builder.message_new(:get, 0, 1)
          builder.channel_call
          builder.pop
        end
      end
      def transform_nth_ref(num)
        # TODO: This is a very bad stub. Fix me.
        if (num == 0)
          builder.push('bin/c9.rb')
        else
          # Totally wrong. Needs to pull from regex state.
          transform_gvar(:"$#{num}")
        end
      end
      def transform_back_ref(name)
        transform_gvar(:"$#{name}")
      end

      def transform_iasgn(name, val = nil)
        if (val.nil?)
          builder.frame_get("self")
          builder.swap
          builder.push(name)
          builder.swap
        else
          builder.frame_get("self")
          builder.push(name)
          transform(val)
        end
        builder.message_new(:"ruby_sys:ivar_set", 0, 2)
        builder.channel_call
        builder.pop
      end
      def transform_ivar(name)
        builder.frame_get("self")
        builder.push(name)
        builder.message_new(:"ruby_sys:ivar_get", 0, 1)
        builder.channel_call
        builder.pop
      end
      def transform_cvar(name)
        builder.frame_get("self")
        builder.push(name)
        builder.message_new(:class_variable_get, 0, 1)
        builder.channel_call
        builder.pop
      end
      def transform_cvdecl(name, val = nil)
        if (val)
          const_self
          builder.push(name)
          transform(val)
          builder.message_new(:class_variable_decl, 0, 2)
          builder.channel_call
          builder.pop
        else
          const_self
          builder.swap
          builder.push(name)
          builder.swap
          builder.message_new(:class_variable_decl, 0, 2)
          builder.channel_call
          builder.pop
        end
      end
      def transform_cvasgn(name, val = nil)
        if (val)
          const_self
          builder.push(name)
          transform(val)
          builder.message_new(:class_variable_set, 0, 2)
          builder.channel_call
          builder.pop
        else
          const_self
          builder.swap
          builder.push(name)
          builder.swap
          builder.message_new(:class_variable_set, 0, 2)
          builder.channel_call
          builder.pop
        end
      end

      def transform_block(*lines)
        if (lines.empty?)
          transform_nil
        else
          count = lines.length
          lines.each_with_index do |line, idx|
            transform(line)
            builder.pop if (count != idx + 1)
          end
        end
      end

      def has_splat(arglist)
        arglist.each_with_index do |arg, idx|
          return idx if arg.first == :splat
        end
        return false
      end

      def transform_call(target, method, arglist, has_iter = false, extra = false)
        if (target.nil? && method == :block_given?)
          builder.frame_get("yield")
          builder.is_not(Primitive::Undef)
          return
        elsif (target.nil? && method == :undefined)
          builder.push(Primitive::Undef)
          return
        end
        if (match = method.to_s.match(/^__c9_(.+)__$/))
          if (match[1] == 'debugger')
            builder.debugger(arglist[1][1])
            builder.push nil
            return
          else
            method = :"ruby_sys:#{match[1]}"
          end
        end

        if (target.nil?)
          transform_self()
        else
          transform(target)
        end

        if (has_iter || extra)
          # If we were called with has_iter == true, an iterator
          # will be sitting on the stack, and we want to swap it in
          # to first sysarg position.
          # Likewise if there's an extra item on the stack to be appended
          # to the message, though it will wind up in the normal args.
          builder.swap
        end

        _, *arglist = arglist

        if (arglist.last && arglist.last[0] == :block_pass)
          block_arg = arglist.pop.dup
          has_iter = true
          transform(block_arg[1])
          builder.message_new(:to_proc_prim, 0, 0)
          builder.channel_call
          builder.pop
        end

        if (first_splat = has_splat(arglist))
          i = 0
          while (i < first_splat)
            transform(arglist.shift)
            i += 1
          end
          builder.message_new(method, has_iter ? 1 : 0, i)
          transform(arglist.shift[1])
          builder.message_new(:to_tuple_prim, 0, 0)
          builder.channel_call
          builder.pop
          builder.message_splat
        else
          arglist.each do |arg|
            transform(arg)
          end

          builder.message_new(method, has_iter ? 1 : 0, arglist.length)
        end
        if (extra)
          builder.swap
          builder.message_add(1)
        end
        builder.channel_call
        builder.pop
      end

      def transform_attrasgn(target, method, arglist = nil)
        if (arglist)
          # extra means there's an extra argument on the stack that should
          # go on the end.
          transform_call(target, method, arglist, false, @state[:masgn])
        else
          # This is for masgn (including proc args)
          transform(target)
          builder.swap # get the arg in place
          builder.message_new(method, 0, 1)
          builder.channel_call
          builder.pop
        end
      end

      def transform_next(val = nil)
        linfo = @state[:loop]
        if (linfo)
          case linfo[:type]
          when :block
            builder.frame_get(linfo[:ret])
            if (val)
              transform(val)
            else
              builder.push nil
            end
            builder.channel_ret
            return
          when :while
            builder.jmp(linfo[:beg_label])
            return
          end
        end
        raise_error :NotImplementedError, "Invalid (or unimplemented?) location for a next"
      end

      def transform_break(val = nil)
        linfo = @state[:loop]
        if (linfo)
          case linfo[:type]
          when :while
            builder.jmp(linfo[:end_label])
            return
          when :block
            @state[:needs_break][0] = true
            builder.channel_special(:unwinder)
            if (val.nil?)
              transform_nil
            else
              transform(val)
            end
            builder.message_new(:iter_break, 0, 1)
            builder.channel_call
            builder.pop
            return
          end
        end
        raise_error :NotImplementedError, "Invalid (or unimplemented?) location for a break"
      end
      def transform_redo
        linfo = @state[:loop]
        if (linfo)
          builder.jmp(linfo[:redo_label])
        else
          raise_error :NotImplementedError, "Invalid (or unimplemented?) location for a redo"
        end
      end
      def transform_retry
        if (@state[:rescue_retry])
          builder.jmp(@state[:rescue_retry])
        else
          raise_error :NotImplementedError, "Invalid (or unimplemented?) location for a retry"
        end
      end

      def transform_for(from, args, block = nil)
        transform_iter(s(:call, from, :each, s(:arglist)), args, block, false)
      end

      def transform_0
        # TODO: This should actually blow up if the value passed
        # to it isn't empty or nil. For now just pop it.
        builder.pop
      end

      # The sexp for this is weird. It embeds the call into
      # the iterator, so we build the iterator and then push it
      # onto the stack, then flag the upcoming call sexp so that it
      # swaps it in to the correct place.
      def transform_iter(call, args, block = nil, new_scope = true)
        needs_break_handler = [false]

        call = call.dup
        call << true

        label_prefix = builder.make_label("Iter:#{call[2]}")
        body_label = label_prefix + ".body"
        no_const_self_label = label_prefix + ".no_const_self"
        args_label = label_prefix + ".args"
        done_label = label_prefix + ".done"

        builder.jmp(done_label)

        builder.set_label(body_label)
        builder.frame_set(label_prefix + ".ret")

        # if we got a self through the sys args,
        # we've been re-bound to a method so use that instead.
        builder.message_sys_unpack(4)
        builder.dup_top
        builder.is(Primitive::Undef)
        builder.jmp_if(args_label)
        builder.frame_set("self")
        builder.frame_set("super")
        builder.pop # block
        builder.dup_top
        builder.is(Primitive::Undef)
        builder.jmp_if(no_const_self_label)
        builder.frame_set("const-self")
        builder.push(nil)
        builder.push(nil)
        builder.push(nil)
        builder.push(nil)
        builder.set_label(args_label)
        builder.pop
        builder.pop
        builder.pop
        builder.set_label(no_const_self_label)
        builder.pop

        with_linked_vtable do
          builder.lexical_linked_scope if (new_scope)
          if (args.nil? || args == 0)
            # no args, pop the message off the stack.
            # TODO: Deal with {|| ... }, if lambda raises for arg count mismatch.
            builder.pop
          else
            if (args[0] == :lasgn || args[0] == :gasgn || args[0] == :attrasgn)
              # Ruby's behaviour on a single arg block is ugly.
              # If it takes one argument, but is given multiple,
              # it's as if it were a single arg splat. Otherwise,
              # it's like a normal method invocation.
              builder.message_count
              builder.is_not(1)
              builder.jmp_if(label_prefix + ".splatify")
              builder.message_unpack(1, 0, 0)
              builder.jmp(label_prefix + ".done_unpack")
              builder.set_label(label_prefix + ".splatify")
              builder.message_unpack(0, 1, 0)
              builder.set_label(label_prefix + ".done_unpack")
            else
              builder.message_count
              builder.is(1)
              builder.jmp_if(label_prefix + ".arrayify")
              builder.message_unpack(0, 1, 0) # splat it all for the masgn
              builder.jmp(label_prefix + ".done_unpack")
              builder.set_label(label_prefix + ".arrayify")
              builder.message_unpack(1, 0, 0)
              builder.message_new(:to_tuple_prim, 0, 0)
              builder.channel_call
              builder.pop
              builder.set_label(label_prefix + ".done_unpack")
            end
            transform(args) # comes in as an lasgn or masgn
            builder.pop
            builder.pop
          end

          if (block.nil?)
            transform_nil()
          else
            linfo = {
              :type => :block,
              :ret => label_prefix + ".ret",
              :redo_label => label_prefix + ".retry",
            }
            with_state(:block => true, :loop => linfo, :needs_break => needs_break_handler) do
              builder.set_label(label_prefix + ".retry")
              transform(block)
            end
          end
        end

        builder.frame_get(label_prefix + ".ret")
        builder.swap
        builder.channel_ret

        builder.set_label(done_label)

        builder.channel_new(body_label)

        if (needs_break_handler.first)
          # But first, we need to set up a trampoline and unwind
          # handler for breaks in the block. This is very complex,
          # and should probably only be done if there actually is a
          # break in the block or the block is passed as an & (so can't
          # be detected).
          breaker_tramp = builder.make_label("call.iter.trampoline")
          breaker_tramp_done = builder.make_label("call.iter.trampoline.done")
          breaker_unwind = builder.make_label("call.iter.unwinder")
          breaker_unwind_ret = builder.make_label("call.iter.unwinder.next")
          builder.jmp(breaker_tramp_done)

          builder.set_label(breaker_tramp)
          builder.frame_set("iter.trampoline.ret")

          builder.channel_special(:unwinder)
          builder.channel_new(breaker_unwind)
          builder.channel_call
          builder.pop
          builder.pop

          builder.frame_get("iter.trampoline.real")
          builder.swap
          builder.channel_call
          builder.pop

          # get here, unwinder needs removal.
          builder.channel_special(:unwinder)
          builder.push(nil)
          builder.channel_call
          builder.pop
          builder.pop

          builder.frame_get("iter.trampoline.ret")
          builder.swap
          builder.channel_ret

          builder.set_label(breaker_unwind)
          builder.local_set(:unwind_return)
          builder.message_id
          builder.is(Primitive::MessageID.new("iter_break"))
          builder.jmp_if(breaker_unwind_ret)
          builder.pop
          builder.local_get(:unwind_return)
          builder.push(false)
          builder.channel_ret

          builder.set_label(breaker_unwind_ret)
          builder.message_unpack(1,0,0)
          builder.swap
          builder.pop
          builder.frame_get("iter.trampoline.ret")
          builder.swap
          builder.channel_ret

          builder.set_label(breaker_tramp_done)
          builder.frame_set("iter.trampoline.real") #will be picked up in trampoline's frame.
          builder.channel_new(breaker_tramp)
        end

        transform(call)
      end

      def transform_resbody(comparisons, body)
        found_label = builder.make_label("resbody.found")
        next_label = builder.make_label("resbody.next")
        done_label = @state[:rescue_done]

        comparisons = comparisons.dup
        comparisons.shift

        err_assign = nil
        last = comparisons.last
        if (last && (last[0] == :lasgn || last[0] == :gasgn || last[0] == :iasgn))
          err_assign = comparisons.pop
        end

        comparisons.each do |cmp|
          builder.dup_top # value from enclosing case.
          transform(cmp)
          builder.swap
          builder.message_new(:===, 0, 1)
          builder.channel_call
          builder.pop
          builder.jmp_if(found_label)
          builder.jmp(next_label)
        end
        builder.set_label(found_label)
        builder.pop
        if (err_assign)
          transform(err_assign)
          # if there is no body, but there is an error assign, it
          # basically is the body.
          if (!body.nil?)
            builder.pop
          end
        end

        if (body.nil?)
          transform_nil
        else
          transform(body)
        end
        builder.jmp(done_label)
        builder.set_label(next_label)
      end

      def transform_rescue(try, *handlers)
        prefix = builder.make_label("rescue") + ":"
        try_label = prefix + "try"
        rescue_label = prefix + "rescue"
        retry_label = prefix + "retry"
        not_raise_label = prefix + "not_raise"
        handled_label = prefix + "handled"
        done_label = prefix + "done"

        else_body = nil

        if (handlers.last && handlers.last.first != :resbody)
          else_body = handlers.pop
        end

        builder.channel_new(try_label)
        builder.push(nil)
        builder.channel_call
        builder.pop
        builder.jmp(done_label)

        builder.set_label(try_label)
        builder.frame_set("rescue-return")
        builder.pop

        # Set the unwinder.
        builder.channel_special(:unwinder)
        builder.channel_new(rescue_label)
        builder.channel_call
        builder.pop
        builder.pop

        # do the work
        builder.set_label(retry_label)

        unwinders = @state[:unwinders] || 0
        with_state(:unwinders => unwinders + 1) do
          transform(try)
        end

        with_state(:rescue_retry=>retry_label) do
          # if we get here, unset the unwinder and jump to the end.
          builder.channel_special(:unwinder)
          builder.push(nil)
          builder.channel_call
          builder.pop
          builder.pop

          # if there was an else handler, call it.
          if (else_body)
            transform(else_body)
            builder.pop
          end

          builder.jmp(handled_label)

          builder.set_label(rescue_label)

          # save the return handler and check the message to see
          # if this is an error or if we should just call the next
          # unwinder.
          builder.local_set("unwind-return")
          # save the message
          builder.dup_top
          builder.local_set("unwind-message")

          builder.message_id
          builder.is(Primitive::MessageID.new("raise"))
          builder.jmp_if_not(not_raise_label)

          builder.message_unpack(1,0,0)
          builder.swap
          builder.pop

          with_state(:rescue_done => handled_label) do
            handlers.each do |handler|
              transform(handler)
            end
          end
        end

        builder.set_label(not_raise_label)
        builder.pop
        builder.local_get("unwind-return")
        builder.push(false)
        builder.channel_ret

        builder.set_label(handled_label)
        builder.frame_get("rescue-return")
        builder.swap
        builder.channel_ret

        builder.set_label(done_label)
      end

      def transform_ensure(body, ens)
        label_prefix = builder.make_label("ensure") + ":"
        begin_label = label_prefix + "begin"
        unwinder_label = label_prefix + "unwinder"
        skip_ens_label = label_prefix + "skip-ensure"
        ens_label = label_prefix + "ensure"
        done_label = label_prefix + "done" # done handling the ensure block
        end_label = label_prefix + "end" # done the whole thing

        builder.channel_new(begin_label)
        builder.push(nil)
        builder.channel_call
        builder.pop
        builder.jmp(end_label)

        builder.set_label(begin_label)
        builder.frame_set("ensure-return")
        builder.pop

        builder.channel_special(:unwinder)
        builder.channel_new(unwinder_label)
        builder.channel_call
        builder.pop
        builder.pop

        unwinders = @state[:unwinders] || 0
        with_state(:ensure => ens_label, :unwinders => unwinders + 1) do
          transform(body)
        end
        # if the body executes correctly, we push
        # nil onto the stack so that the ensure 'channel'
        # picks that up as the return path. If it
        # does, when it goes to return, it will just
        # jmp to the done label rather than calling
        # the next handler.
        builder.push(nil)
        # clear the unwinder
        builder.channel_special(:unwinder)
        builder.push(nil)
        builder.channel_call
        builder.pop
        builder.pop

        builder.jmp(ens_label)
        builder.set_label(unwinder_label)

        # check if the message is :query. If it is, don't
        # run the unwind.
        builder.swap
        builder.message_id
        builder.is(Primitive::MessageID.new("query"))
        builder.local_set "skip-ensure"
        builder.swap
        builder.local_get "skip-ensure"
        builder.jmp_if(skip_ens_label)

        builder.set_label(ens_label)

        # run the ensure block
        transform(ens)
        builder.pop

        builder.set_label(skip_ens_label)
        # if we came here via a call (non-nil return path),
        # pass on to the next unwind handler rather than
        # leaving by the done label.
        builder.dup_top
        builder.jmp_if_not(done_label)
        builder.push(nil)
        builder.channel_ret

        builder.set_label(done_label)
        builder.pop
        builder.frame_get("ensure-return")
        builder.swap
        builder.channel_ret

        builder.set_label(end_label)
      end

      def transform_defined(val)
        case val[0]
        when :const, :colon3
          done_label = builder.make_label("defined.const.done")
          transform(val)
          builder.dup_top
          builder.jmp_if_not(done_label)
          builder.pop
          builder.push(:constant)
          builder.set_label(done_label)
        when :call
          done_label = builder.make_label("defined.call.done")
          name, lhs, name, args = val
          # TODO: This will error if lhs can't evaluate,
          # and that needs to be fixed.
          if (lhs)
            transform(lhs)
          else
            transform_self
          end
          builder.dup_top
          builder.jmp_if_not(done_label)
          builder.push(name)
          builder.message_new(:respond_to?, 0, 1)
          builder.channel_call
          builder.pop
          builder.dup_top
          builder.jmp_if_not(done_label)
          builder.pop
          builder.push(:method)
          builder.set_label(done_label)
        when :ivar
          transform_self
          name, varname = val
          transform_lit(varname)
          builder.message_new(:instance_variable?, 0, 1)
          builder.channel_call
          builder.pop
        else
          raise_error :NotImplementedError, "Unknown defined? type #{val[0]}."
        end
      end

      def transform_match(match_str)
        transform_colon3(:Regexp)
        transform(match_str)
        builder.message_new(:new, 0, 1)
        builder.channel_call
        builder.pop
      end

      def transform_eval(body)
        eval_prefix = builder.make_label("eval")
        body_label = eval_prefix + ":body"

        builder.frame_set("return")
        if (!body.nil?)
          builder.message_sys_unpack(4)
          builder.frame_set("self")
          builder.pop # super
          builder.pop # block
          builder.dup_top
          builder.jmp_if(body_label)

          builder.pop
          builder.push(nil)
          builder.channel_special(:Object)
          builder.tuple_new(2)

          builder.set_label(body_label)
          builder.frame_set("const-self")
          builder.pop

          with_new_vtable do
            transform(body)
          end
        else
          builder.pop
          transform_nil
        end
        builder.frame_get("return")
        builder.swap
        builder.channel_ret
      end

      def transform_file(body)
        builder.local_set("script-return")
        builder.pop
        builder.channel_special(:global_self)
        builder.frame_set("self")
        if (!body.nil?)
          builder.push(nil)
          builder.channel_special(:Object)
          builder.tuple_new(2)
          builder.frame_set("const-self")
          with_new_vtable do
            transform(body)
          end
        else
          transform_nil
        end
        builder.pop
        builder.local_get("script-return")
        builder.push(true)
        builder.channel_ret
      end

      def method_missing(name, *args)
        if (match = name.to_s.match(%r{^transform_(.+)$}))
          puts "Unknown parse tree #{match[1]}:"
          pp args
        end
        super
      end

      def current_sexp
        Thread.current[:cur_sexp]
      end
      def with_current_sexp(tree)
        if (tree.is_a? Sexp)
          begin
            if (!current_sexp || current_sexp.line != tree.line && tree.line)
              builder.line(tree.file.to_s, tree.line)
            end
            old, Thread.current[:cur_sexp] = current_sexp, tree
            yield
          ensure
            Thread.current[:cur_sexp] = old
          end
        else
          yield
        end
      end

      def transform(tree)
        begin
          with_current_sexp(tree) do
            name, *info = tree
            send(:"transform_#{name}", *info)
          end
        rescue
          cur = Thread.current[:cur_sexp]
          puts "Compile error near line #{cur.line} of #{cur.file}" if cur
          raise
        end
      end
    end
  end
end
