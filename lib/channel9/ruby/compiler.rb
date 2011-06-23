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
      def find_local_depth(name, add = false)
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
          transform_const(:Regexp)
          builder.push(Primitive::String.new(literal.to_s))
          builder.message_new(:new, 0, 1)
          builder.channel_call
          builder.pop
        else
          builder.push literal
        end
      end
      def transform_dot2(first, last)
        transform_const(:Range)
        builder.push(false)
        transform(last)
        transform(first)
        builder.message_new(:new, 0, 3)
        builder.channel_call
        builder.pop
      end
      def transform_dot3(first, last)
        transform_const(:Range)
        builder.push(true)
        transform(last)
        transform(first)
        builder.message_new(:new, 0, 3)
        builder.channel_call
        builder.pop
      end

      def transform_str(str)
        transform_const(:String)
        builder.push(Primitive::String.new(str))
        builder.message_new(:new, 0, 1)
        builder.channel_call
        builder.pop        
      end
      def transform_evstr(ev)
        transform(ev)
      end
      def transform_dsym(initial, *strings)
        strings.reverse.each do |str|
          transform(str)
        end
        if (initial.length > 0)
          transform_str(initial)
          builder.string_new(:to_s_prim, 1 + strings.length)
        else
          builder.string_new(:to_s_prim, strings.length)
        end
      end
      def transform_dstr(initial, *strings)
        transform_const(:String)
        transform_dsym(initial, *strings)
        builder.message_new(:new, 0, 1)
        builder.channel_call
        builder.pop
      end
      def transform_dregx(initial, *strings)
        transform_const(:Regexp)
        transform_dsym(initial, *strings)
        builder.message_new(:new, 0, 1)
        builder.channel_call
        builder.pop
      end
      def transform_hash(*items)
        transform_const(:Hash)
        items.reverse.each do |item|
          transform(item)
        end
        builder.tuple_new(items.length)
        builder.message_new(:new, 0, 1)
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

        transform_const(:Array)
        items.reverse.each do |item|
          transform(item)
        end
        builder.tuple_new(items.length)
        transform(splat) if splat
        builder.message_new(:new, 0, 1)
        builder.channel_call
        builder.pop
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
        builder.push(nil.to_c9)
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
        builder.push(nil.to_c9)
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

      def transform_op_asgn1(var_if, args, op, val)
        done_label = builder.make_label("asgn1.done")
        transform_call(var_if, :[], args)
        builder.dup_top
        case op
        when :'||'
          builder.jmp_if(done_label)
        when :'&&'
          builder.jmp_if_not(done_label)
        else
          raise "Unknown operator #{op}"
        end
        builder.pop
        asgn_args = args + s(val)
        transform_call(var_if, :[]=, asgn_args)
        builder.set_label(done_label)
      end

      def transform_when(comparisons, body)
        found_label = builder.make_label("when.found")
        next_label = builder.make_label("when.next")
        done_label = @state[:case_done]

        comparisons = comparisons.dup
        comparisons.shift

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

        builder.pop # value not needed anymore.
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

        if (defargs.length == 0)
          builder.message_unpack(args.length, splatarg ? 1 : 0, 0)
          args.each do |arg|
            builder.local_set(find_local_depth(arg, true), arg)
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
            builder.local_set(find_local_depth(args[i], true), args[i])
            i += 1
          end

          while (i < args.length)
            builder.frame_get("arg.count")
            builder.is(i)
            builder.jmp_if(defarg_labels[i-must_have])
            builder.local_set(find_local_depth(args[i], true), args[i])
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
          transform_const(:Array)
          builder.swap
          builder.message_new(:new, 0, 1)
          builder.channel_call
          builder.pop
          builder.local_set(find_local_depth(splatarg, true), splatarg)
        end
        if (blockarg)
          builder.frame_get("yield")
          builder.local_set(find_local_depth(blockarg, true), blockarg)
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

      def need_return_unwind(code)
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
          compiler.with_state(:need_long_return => need_info) do
            compiler.transform(code)
            return need_info[0]
          end
        end
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
        builder.local_clean_scope
        builder.frame_set("return")
        
        if (nru = need_return_unwind(code))
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

          with_state(:has_long_return => nru, :name => name) do
            transform(code)
          end
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
          builder.pop # -> unwind_message
          builder.message_name # -> name -> um
          builder.is(:long_return) # -> is -> um
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
          builder.channel_special(:unwinder) # -> unwinder -> um
          builder.swap # -> um -> unwinder
          builder.channel_ret
        end
        builder.set_label(method_done_label)
        if (on.nil?)
          builder.frame_get("self")
        else
          transform(on)
        end
        builder.channel_new(method_label)
        builder.push(name)
        builder.message_new(on.nil? ? :define_method : :define_singleton_method, 1, 1)
        builder.channel_call
        builder.pop
      end

      def transform_super(*args)
        builder.frame_get("super")
        args.each do |arg|
          transform(arg)
        end
        builder.message_new(@state[:name], 0, args.length)
        builder.channel_call
        builder.pop
      end

      def transform_zsuper
        builder.frame_get("super")
        builder.push(nil)
        builder.channel_call
        builder.pop
      end

      def transform_yield(*args)
        builder.frame_get("yield")

        args.each do |arg|
          transform(arg)
        end
        builder.message_new(:call, 0, args.length)
        builder.channel_call
        builder.pop
      end

      def transform_return_vals(vals)
        if (vals.any?)
          if (vals.length == 1)
            if (vals[0][0] == :svalue)
              # basic effect of a single splatted value
              # in a return statement is to turn no or
              # one value into unpacking the first value
              # from the tuple, otherwise just return the tuple.
              splat_done = builder.make_label("return.splat.done")
              splat_to_a = builder.make_label("return.splat.to_a")
              transform(vals[0][1][1]) # there's a :splat node in here too.
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
              builder.message_new(:to_a, 0, 1)
              builder.channel_call
              builder.pop
              builder.set_label(splat_done)
            else
              transform(vals[0])
            end
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
        if (@state[:ensure] || @state[:block])
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
        builder.jmp(done_label)

        builder.set_label(body_label)
        builder.local_clean_scope
        builder.frame_set("return")
        builder.message_sys_unpack(1)
        builder.frame_set("self")
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
        builder.channel_new(body_label)
        builder.push(:__sbody__)
        builder.message_new(:define_singleton_method, 1, 1)
        builder.channel_call
        builder.pop
        builder.pop

        builder.message_new(:__sbody__, 0, 0)
        builder.channel_call
        builder.pop
      end

      def transform_class(name, superclass, body)
        label_prefix = "Class:#{name}"
        body_label = builder.make_label(label_prefix + ".body")
        done_label = builder.make_label(label_prefix + ".done")

        full_name = @state[:static_scope] + [name]
        full_name = full_name.join('::')

        # See if it's already there
        transform_const(name)
        builder.dup_top
        builder.jmp_if(done_label)

        # If it's not, make a new class and set it.
        builder.pop
        load_static_scope
        builder.push(name)
        
        builder.channel_special(:Class)
        if (superclass.nil?)
          builder.channel_special(:Object)
        else
          transform(superclass)
        end
        builder.push(full_name.to_sym)
        builder.message_new(:new, 0, 2)
        builder.channel_call
        builder.pop

        builder.message_new(:const_set, 0, 2)
        builder.channel_call
        builder.pop
        builder.jmp(done_label)

        builder.set_label(body_label)
        builder.local_clean_scope
        builder.frame_set("return")
        builder.message_sys_unpack(1)
        builder.frame_set("self")
        builder.pop
        with_state(:static_scope => @state[:static_scope] + [name]) do
          if (body.nil?)
            transform_nil
          else
            with_new_vtable do
              transform(body)
            end
          end
        end
        builder.frame_get("return")
        builder.swap
        builder.channel_ret

        builder.set_label(done_label)

        builder.dup_top
        builder.channel_new(body_label)
        builder.push(:__body__)
        builder.message_new(:define_singleton_method, 1, 1)
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

        full_name = @state[:static_scope] + [name]
        full_name = full_name.join('::')

        # See if it's already there
        transform_const(name)
        builder.dup_top
        builder.jmp_if(done_label)

        # If it's not, make a new module and set it.
        builder.pop
        load_static_scope
        builder.push(name)
        
        builder.channel_special(:Module)
        builder.push(full_name.to_sym)
        builder.message_new(:new, 0, 1)
        builder.channel_call
        builder.pop

        builder.message_new(:const_set, 0, 2)
        builder.channel_call
        builder.pop
        builder.jmp(done_label)

        builder.set_label(body_label)
        builder.local_clean_scope
        builder.frame_set("return")
        builder.message_sys_unpack(1)
        builder.frame_set("self")
        builder.pop
        with_state(:static_scope => @state[:static_scope] + [name]) do
          if (body.nil?)
            transform_nil
          else
            with_new_vtable do
              transform(body)
            end
          end
        end
        builder.frame_get("return")
        builder.swap
        builder.channel_ret

        builder.set_label(done_label)

        builder.dup_top
        builder.channel_new(body_label)
        builder.push(:__body__)
        builder.message_new(:define_singleton_method, 1, 1)
        builder.channel_call
        builder.pop
        builder.pop

        builder.message_new(:__body__, 0, 0)
        builder.channel_call
        builder.pop
      end

      def transform_alias(first, second)
        builder.frame_get("self")
        transform(first)
        transform(second)
        builder.message_new(:alias_method, 0, 2)
        builder.channel_call
        builder.pop
      end

      def load_static_scope
        builder.channel_special(:Object)
        if (@state[:static_scope].any?)
          @state[:static_scope].each do |scope|
            builder.push(scope)
          end
          builder.message_new(:const_get_scoped, 0, @state[:static_scope].length)
          builder.channel_call
          builder.pop
        end
      end

      def transform_cdecl(name, val)
        load_static_scope
        builder.push(name)
        transform(val)
        builder.message_new(:const_set, 0, 2)
        builder.channel_call
        builder.pop
      end
      def transform_const(name)
        builder.channel_special(:Object)
        @state[:static_scope].each do |scope|
          builder.push(scope)
        end
        builder.push(name)
        builder.message_new(:const_get_scoped, 0, @state[:static_scope].length + 1)
        builder.channel_call
        builder.pop
      end

      def transform_colon3(name)
        builder.channel_special(:Object)
        builder.push(name)
        builder.message_new(:const_get, 0, 1)
        builder.channel_call
        builder.pop
      end

      def transform_colon2(lhs, name)
        transform(lhs)
        builder.push(name)
        builder.message_new(:const_get, 0, 1)
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

        lhs.each do |asgn|
          transform(asgn)
          builder.pop
        end
        if splat
          builder.message_new(:to_a, 0, 0)
          builder.channel_call
          builder.pop
          transform(splat[1])
          builder.pop # don't want the result of the individual assignments.
        end
      end

      # If given a nil value, assumes the rhs is
      # already on the stack.
      def transform_lasgn(name, val = nil)
        transform(val) if !val.nil?
        builder.dup_top
        builder.local_set(find_local_depth(name, true), name)
      end
      def transform_lvar(name)
        builder.local_get(find_local_depth(name), name)
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
          builder.frame_get("self")
          builder.swap
          builder.push(name)
          builder.swap
        else
          builder.frame_get("self")
          builder.push(name)
          transform(val)
        end
        builder.message_new(:instance_variable_set, 0, 2)
        builder.channel_call
        builder.pop
      end
      def transform_ivar(name)
        builder.frame_get("self")
        builder.push(name)
        builder.message_new(:instance_variable_get, 0, 1)
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
      def transform_cvdecl(name, val)
        builder.frame_get("self")
        builder.push(name)
        transform(val)
        builder.message_new(:class_variable_decl, 0, 2)
        builder.channel_call
        builder.pop
      end
      def transform_cvasgn(name, val)
        builder.frame_get("self")
        builder.push(name)
        transform(val)
        builder.message_new(:class_variable_set, 0, 2)
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

      def has_splat(arglist)
        arglist.each_with_index do |arg, idx|
          return idx if arg.first == :splat
        end
        return false
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

        if (arglist.last && arglist.last[0] == :block_pass)
          block_arg = arglist.pop.dup
          has_iter = true
          transform(block_arg[1])
        end

        if (first_splat = has_splat(arglist))
          i = 0
          while (i < first_splat)
            transform(arglist.shift)
            i += 1
          end
          builder.message_new(method.to_c9, has_iter ? 1 : 0, i)
          transform(arglist.shift[1])
          builder.message_new(:to_tuple_prim, 0, 0)
          builder.channel_call
          builder.pop
          builder.message_splat
        else
          arglist.each do |arg|
            transform(arg)
          end

          builder.message_new(method.to_c9, has_iter ? 1 : 0, arglist.length)
        end
        builder.channel_call
        builder.pop
      end

      def transform_attrasgn(target, method, arglist)
        transform_call(target, method, arglist)
      end

      def transform_next(val = nil)
        linfo = @state[:loop]
        case linfo[:type]
        when :block
          builder.frame_get(linfo[:ret])
          if (val)
            transform(val)
          else
            builder.push nil
          end
          builder.channel_ret
        when :while
          builder.jmp(linfo[:beg_label])
        else
          raise "Invalid (or unimplemented?) location for a next: #{linfo[:type]}"
        end
      end

      def transform_break(val = nil)
        linfo = @state[:loop]
        case linfo[:type]
        when :while
          builder.jmp(linfo[:end_label])
        else
          raise "Invalid (or unimplemented?) location for a break"
        end
      end
      def transform_redo
        linfo = @state[:loop]
        builder.jmp(linfo[:redo_label])
      end
      def transform_retry
        if (@state[:rescue_retry])
          builder.jmp(@state[:rescue_retry])
        else
          raise "Invalid (or unimplemented?) location for a retry"
        end
      end

      # The sexp for this is weird. It embeds the call into
      # the iterator, so we build the iterator and then push it
      # onto the stack, then flag the upcoming call sexp so that it
      # swaps it in to the correct place.
      def transform_iter(call, args, block = nil)
        call = call.dup
        call << true

        label_prefix = builder.make_label("Iter:#{call[2]}")
        body_label = label_prefix + ".body"
        args_label = label_prefix + ".args"
        done_label = label_prefix + ".done"

        builder.jmp(done_label)

        builder.set_label(body_label)
        builder.frame_set(label_prefix + ".ret")

        # if we got a self through the sys args, 
        # we've been re-bound to a method so use that instead.
        builder.message_sys_unpack(1)
        builder.dup_top
        builder.is(Primitive::Undef)
        builder.jmp_if(args_label)
        builder.frame_set("self")
        builder.push(nil)
        builder.set_label(args_label)
        builder.pop

        with_linked_vtable do
          builder.local_linked_scope
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
            with_state(:block => true, :loop => linfo) do
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

        transform(call)
      end

      def transform_resbody(comparisons, body)
        found_label = builder.make_label("resbody.found")
        next_label = builder.make_label("resbody.next")
        done_label = @state[:rescue_done]

        comparisons = comparisons.dup
        comparisons.shift

        err_assign = nil
        if (comparisons.last && comparisons.last[0] == :lasgn)
          err_assign = comparisons.pop[1]
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
        if (err_assign)
          builder.local_set(find_local_depth(err_assign, true), err_assign)
        else
          builder.pop
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
        try_label = builder.make_label("try")
        rescue_label = builder.make_label("rescue")
        retry_label = builder.make_label("rescue.retry")
        not_raise_label = builder.make_label("rescue.not_raise")
        done_label = builder.make_label("rescue.done")

        # Set the unwinder.
        builder.channel_special(:unwinder)
        builder.channel_new(rescue_label)
        builder.channel_call
        builder.pop
        builder.pop

        # do the work
        builder.set_label(retry_label)
        with_state(:rescue_retry=>retry_label) do
          transform(try)

          # if we get here, unset the unwinder and jump to the end.
          builder.channel_special(:unwinder)
          builder.push(nil)
          builder.channel_call
          builder.pop
          builder.pop
          builder.jmp(done_label)

          builder.set_label(rescue_label)

          # pop the return handler and check the message to see
          # if this is an error or if we should just call the next
          # unwinder.
          builder.pop
          builder.message_name
          builder.is(:raise.to_c9)
          builder.jmp_if_not(not_raise_label)

          builder.message_unpack(1,0,0)

          with_state(:rescue_done => done_label) do
            handlers.each do |handler|
              transform(handler)
            end
          end
        end

        builder.pop
        builder.set_label(not_raise_label)
        builder.channel_special(:unwinder)
        builder.swap
        builder.channel_ret

        builder.set_label(done_label)
        # get rid of the unwind message.
        builder.swap
        builder.pop
      end

      def transform_ensure(body, ens)
        ens_label = builder.make_label("ensure")
        done_label = builder.make_label("ensure.done")

        builder.channel_special(:unwinder)
        builder.channel_new(ens_label)
        builder.channel_call
        builder.pop
        builder.pop

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
        # clear the unwinder
        builder.channel_special(:unwinder)
        builder.push(nil)
        builder.channel_call
        builder.pop
        builder.pop

        builder.set_label(ens_label)

        # run the ensure block
        transform(ens)
        builder.pop

        # if we came here via a call (non-nil return path),
        # pass on to the next unwind handler rather than
        # leaving by the done label.
        builder.jmp_if_not(done_label)
        builder.channel_special(:unwinder)
        builder.swap
        builder.channel_ret

        builder.set_label(done_label)
      end

      def transform_file(body)
        builder.frame_set("return")
        builder.frame_set("self")
        if (!body.nil?)
          with_state(:static_scope => []) do
            with_new_vtable do
              transform(body)
            end
          end
        else
          transform_nil
        end
        builder.frame_get("return")
        builder.swap
        builder.channel_ret
      end

      def method_missing(name, *args)
        if (match = name.to_s.match(%r{^transform_(.+)$}))
          puts "Unknown parse tree #{match[1]}:"
          pp args
        end
        super
      end

      def transform(tree)
        begin
          Thread.current[:cur_sexp] = tree if (tree.is_a? Sexp)
          name, *info = tree
          send(:"transform_#{name}", *info)
        rescue
          cur = Thread.current[:cur_sexp]
          puts "Compile error near line #{cur.line} of #{cur.file}" if cur
          raise
        end
      end
    end
  end
end