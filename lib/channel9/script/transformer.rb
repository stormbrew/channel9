module Channel9
  module Script
    class Transformer < Parslet::Transform
      class Context
        attr :filename
        attr :last_line, true

        def initialize(filename)
          @filename = filename
          @label_prefixes = Hash.new {|h,k| h[k] = 0 }
          @variable_frames = [[0, {}]]
          @lexical_scope_level = 0
        end

        def label_prefix(name)
          num = @label_prefixes[name] += 1
          name + "." + num.to_s + ":"
        end

        def find_var(name)
          return [0, @variable_frames.first[1][name]] if @variable_frames.first[1][name]
          @variable_frames.each do |idx, frame|
            return [@lexical_scope_level - idx, frame[name]] if (frame[name] == 'lexical' || frame[name] == 'frame')
          end
          nil
        end
        def variable_frame(stream, has_lexical_scope, &block)
          begin
            if (has_lexical_scope)
              stream.lexical_linked_scope
              @lexical_scope_level += 1
            end
            @variable_frames.unshift([@lexical_scope_level, {}])
            block.call
          ensure
            @variable_frames.shift
            @lexical_scope_level -= 1 if (has_lexical_scope)
          end
        end
        def declare_var(name, type)
          @variable_frames.first[1][name] = type
        end
      end

      class Node
        attr :line
        attr :col

        def initialize(tree, other = {})
          if (tree)
            @line, @col = tree.line_and_column
          end
          other.each {|k, v|
            send(:"#{k}=", v)
          }
          @other = other
        end
        def line_and_column
          [@line, @col]
        end

        def compile_node(ctx, stream, void)
          line_info = [ctx.filename, line, col]
          if (ctx.last_line != line_info && line && col)
            stream.line(*line_info)
            ctx.last_line = line_info
          end
          compile(ctx, stream, void)
        end

        def visit_child(child, &action)
          if (child.respond_to?(:visit))
            child.visit(&action)
          elsif (child.respond_to?(:to_ary) && child.respond_to?(:each))
            child.each {|i| visit_child(i, &action) }
          else
            action.call(child)
          end
        end

        # visits all child nodes calling action. If action
        # returns stop, it won't continue recursing. To
        # stop all processing, use throw.
        def visit(&action)
          cont = action.call(self)
          if (cont != :stop)
            @other.each {|k,child|
              visit_child(child, &action)
            }
          end
        end
      end

      class CommentNode < Node
        attr_accessor :text

        def compile(ctx, stream, void)
        end
      end

      class ConstantNode < Node
        attr_accessor :val

        def compile(ctx, stream, void)
          stream.push(val) if (!void)
        end
      end
      class NilNode < ConstantNode; end
      class UndefNode < ConstantNode; end
      class TrueNode < ConstantNode; end
      class FalseNode < ConstantNode; end
      class IntegerConstantNode < ConstantNode; end
      class StringConstantNode < ConstantNode; end

      class MessageIdNode < ConstantNode; end
      class ProtocolIdNode < ConstantNode; end
      
      class ListNode < Node
        attr_accessor :items

        def compile(ctx, stream, void)
          if (items.length == 1 && !items.first.is_a?(Node))
            stream.tuple_new(0)
          else
            items.reverse.each do |item|
              item.compile_node(ctx, stream, false)
            end
            stream.tuple_new(items.length)
            stream.pop if (void)
          end
        end
      end

      class FunctionNode < Node
        attr_accessor :args, :msg, :output, :body

        def compile(ctx, stream, void)
          prefix = ctx.label_prefix('func')
          done_label = prefix + "done"
          body_label = prefix + "body"

          output_var = output || LocalDeclareNode.new(nil, :name => 'return', :type => 'frame')

          stream.jmp(done_label)
          stream.set_label(body_label)

          has_lexical_declaration = false
          visit do |node|
            case node
            when self
            when FunctionNode
              :stop # don't look in child nodes.
            when LocalDeclareNode
              has_lexical_declaration = true if (node.type == 'lexical')
            end
          end

          ctx.variable_frame(stream, has_lexical_declaration) do
            output_var.compile_argument(ctx, stream, true)

            if (args && args.first && args.length > 0)
              stream.message_unpack(args.length, 0, 0)

              args.each do |arg|
                arg.compile_argument(ctx, stream, true)
              end
            end

            if (msg.nil?)
              stream.pop # remove the message
            else
              msg.compile_argument(ctx, stream, true)
            end

            body.compile_node(ctx, stream, false)

            output_var.compile_get(ctx, stream, false)
            stream.swap
            stream.channel_ret
          end

          stream.set_label(done_label)
          stream.channel_new(body_label)
          stream.pop if void
        end
      end

      class VariableNode < Node
        attr_accessor :name
      end
      class SpecialVariableNode < VariableNode
        def compile(ctx, stream, void)
          stream.channel_special(name) if (!void)
        end
      end
      class LocalVariableNode < VariableNode 
        def find(ctx)
          ctx.find_var(name)
        end

        # arguments always default to local
        def compile_argument(ctx, stream, void)
          ctx.declare_var(name, "local")
          stream.dup_top if (!void)
          stream.local_set(name)
        end

        def compile_get(ctx, stream, void)
          compile(ctx, stream, void)
        end

        def compile_set(ctx, stream, void)
          var_depth, type = find(ctx)
          raise "Undeclared variable '#{name}' at #{ctx.filename}:#{line}:#{col}" if !var_depth

          case (type)
          when 'lexical'
            stream.lexical_set(var_depth, name)
          when 'local'
            stream.local_set(name)
          when 'frame'
            stream.frame_set(name)
          else
            raise "Unknown variable type #{type}"
          end
        end

        def compile(ctx, stream, void)
          var_depth, type = *find(ctx)
          raise "Undeclared variable '#{name}' at #{ctx.filename}:#{line}:#{col}" if !var_depth
          if (!void)
            case (type)
            when 'lexical'
              stream.lexical_get(var_depth, name)
            when 'local'
              stream.local_get(name)
            when 'frame'
              stream.frame_get(name)
            else
              raise "Unknown variable type #{type}"
            end
          end
        end
      end
      class LocalDeclareNode < LocalVariableNode
        attr_accessor :type
        attr_accessor :expression

        def find(ctx)
          ctx.declare_var(name, type)
          [0, type]
        end

        # compile with the value to be set already on
        # the stack.
        def compile_argument(ctx, stream, void)
          find(ctx)
          stream.dup_top if (!void)
          case (type)
          when 'lexical'
            stream.lexical_set(0, name)
          when 'local'
            stream.local_set(name)
          when 'frame'
            stream.frame_set(name)
          else
            raise "Unknown variable type #{type}"
          end
        end

        def compile_set(ctx, stream, void)
          compile_argument(ctx, stream, void)
        end

        def compile_get(ctx, stream, void)
          var_depth, type = *find(ctx)
          raise "Undeclared variable '#{name}' at #{ctx.filename}:#{line}:#{col}" if !var_depth
          if (!void)
            case (type)
            when 'lexical'
              stream.lexical_get(var_depth, name)
            when 'local'
              stream.local_get(name)
            when 'frame'
              stream.frame_get(name)
            else
              raise "Unknown variable type #{type}"
            end
          end
        end
        def compile(ctx, stream, void)
          if (expression.nil?)
            super
          else
            find(ctx)
            expression.compile_node(ctx, stream, false)
            compile_argument(ctx, stream, void)
          end
        end
      end

      class CallActionNode < Node; end
      class ValueInvokeNode < CallActionNode
        attr_accessor :args

        def compile(ctx, stream, void)
          args.each do |arg|
            arg.compile_node(ctx, stream, false)
          end
          stream.message_new("call", 0, args.length)
        end
      end
      class MemberInvokeNode < CallActionNode
        attr_accessor :name, :args

        def compile(ctx, stream, void)
          args.each do |arg|
            arg.compile_node(ctx, stream, false)
          end
          stream.message_new(name, 0, args.length)
        end
      end

      class CallNode < Node
        attr_accessor :on, :action

        def compile(ctx, stream, void)
          on.compile_node(ctx, stream, false)
          action.compile_node(ctx, stream, false)
          stream.channel_call
          stream.pop # get rid of unwanted return channel
          stream.pop if (void)
        end
      end
      class SendNode < Node
        attr_accessor :target, :expression, :continuation

        def compile(ctx, stream, void)
          # Todo: Make this recognize tail recursion.
          target.compile_node(ctx, stream, false)
          expression.compile_node(ctx, stream, false)
          stream.channel_call
          if (continuation.nil?)
            stream.pop
          else
            continuation.compile_set(ctx, stream, true)
          end
          stream.pop if (void)
        end
      end
      class ReturnNode < Node
        attr_accessor :target, :expression

        def compile(ctx, stream, void)
          # TODO: Make this not have such intimate knowledge
          # of CallNode and SendNode
          case expression
          when CallNode
            expression.on.compile_node(ctx, stream, false)
            target.compile_node(ctx, stream, false)
            expression.action.compile_node(ctx, stream, false)
            stream.channel_send
          else
            target.compile_node(ctx, stream, false)
            expression.compile_node(ctx, stream, false)
            stream.channel_ret
          end
          # no need to deal with void, these don't continue.
        end
      end

      class InstructionNode < Node
        attr_accessor :name, :args

        def compile(ctx, stream, void)
          begin
            stream.send(name, *args.collect {|arg| arg.val })
          rescue => e
            raise "Error compiling instruction #{name}@#{ctx.filename}:#{line}:#{col}: #{e}"
          end
        end
      end

      class BytecodeNode < Node
        attr_accessor :inputs, :instructions

        def compile(ctx, stream, void)
          if (inputs.length == 1 && !inputs.first.is_a?(Node))
            inputs.pop
          end
          inputs.each do |input|
            input.compile_node(ctx, stream, false)
          end
          instructions.each do |instruction|
            instruction.compile_node(ctx, stream, false)
          end
          stream.pop if (void)
        end
      end

      class StatementNode < Node
        attr_accessor :doc, :expression

        def compile(ctx, stream, void)
          expression.compile_node(ctx, stream, void)
        end
      end
      class SequenceNode < Node
        attr_accessor :statements

        def compile(ctx, stream, void)
          if (statements.length == 1 && !statements.first.is_a?(Node))
            stream.push(nil)
          else
            # get rid of a trailing comment node as it'll mess up our
            # calculations for letting the last statement push something
            # onto the stack.
            statements.pop if (statements.last.kind_of?(CommentNode))
            last_idx = statements.length - 1
            statements.each_with_index do |stmt, idx|
              # all but the last statement should be treated as void,
              # and then the last one only if the block as a whole is void.
              stmt.compile_node(ctx, stream, idx != last_idx || void)
            end
          end
        end
      end

      class AssignTargetNode < Node
        attr_accessor :var, :op

        def compile(ctx, stream, void)
          var_depth, type = var.find(ctx)
          raise "Undeclared variable '#{var.name}' at #{ctx.filename}:#{line}:#{col}" if !var_depth

          if (op != '=')
            real_op = op.gsub(/=$/, '')
            stream.message_new(real_op, 0, 1)
            var.compile_node(ctx, stream, false)
            stream.swap
            stream.channel_call
            stream.pop
          end

          stream.dup_top if (!void)
          var.compile_set(ctx, stream, void)
        end
      end
      class AssignmentNode < Node
        attr_accessor :to, :expr

        def compile(ctx, stream, void)
          expr.compile_node(ctx, stream, false)
          to.compile_node(ctx, stream, void)
        end
      end

      class OperatorNode < Node
        attr_accessor :op, :right

        def compile(ctx, stream, void)
          right.compile_node(ctx, stream, false)
          stream.message_new(op, 0, 1)
        end
      end
      class BinOpNode < Node
        attr_accessor :on, :action

        def compile(ctx, stream, void)
          on.compile_node(ctx, stream, false)
          action.compile_node(ctx, stream, false)
          stream.channel_call
          stream.pop
          stream.pop if (void)
        end
      end
      class SumNode < BinOpNode; end
      class ProductNode < BinOpNode; end
      class RelationalNode < BinOpNode; end
      class EqualityNode < BinOpNode
        def compile(ctx, stream, void)
          case action.op
          when '=='
            on.compile_node(ctx, stream, false)
            action.right.compile_node(ctx, stream, false)
            stream.is_eq
            stream.pop if (void)
          when '!='
            on.compile_node(ctx, stream, false)
            action.right.compile_node(ctx, stream, false)
            stream.is_not_eq
            stream.pop if (void)
          else
            super
          end
        end
      end

      class IfNode < Node
        attr_accessor :condition, :block, :else

        def compile(ctx, stream, void)
          prefix = ctx.label_prefix('if')
          done_label = prefix + "done"
          else_label = prefix + "else"

          condition.compile_node(ctx, stream, false)
          stream.jmp_if_not(else_label)
          
          block.compile_node(ctx, stream, void)
          stream.jmp(done_label)
          
          stream.set_label(else_label)
          if (!self.else.nil?)
            self.else.compile_node(ctx, stream, void)
          else
            stream.push(nil) if (!void)
          end
          stream.set_label(done_label)
        end
      end

      class WhileNode < Node
        attr_accessor :condition, :block

        def compile(ctx, stream, void)
          prefix = ctx.label_prefix('while')
          start_label = prefix + "start"
          done_label = prefix + "done"

          stream.set_label(start_label)
          condition.compile_node(ctx, stream, false)
          stream.jmp_if_not(done_label)
          block.compile_node(ctx, stream, true)
          stream.jmp(start_label)
          stream.set_label(done_label)

          stream.push(nil) if (!void)
        end
      end

      class CaseNode < Node
        attr_accessor :value, :block


      end

      class SwitchNode < Node
        attr_accessor :condition, :cases, :else

        def compile(ctx, stream, void)
          prefix = ctx.label_prefix("switch")
          else_label = prefix + "else"
          done_label = prefix + "done"

          labels = cases.collect do |c|
            prefix + c.value.val
          end
          if (labels.uniq != labels)
            raise "Duplicate label in switch statement #{ctx.file}:#{ctx.line}:#{ctx.column}"
          end

          condition.compile_node(ctx, stream, false)
          cases.each_with_index do |c, idx|
            stream.dup_top
            stream.is(c.value.val)
            stream.jmp_if(labels[idx])
          end
          stream.pop # get rid of the test value.
          stream.jmp(else_label)
          cases.each_with_index do |c, idx|
            stream.set_label(labels[idx])
            stream.pop # get rid of the test value.
            c.block.compile_node(ctx, stream, void)
            stream.jmp(done_label)
          end

          stream.set_label(else_label)
          if (self.else)
            self.else.compile_node(ctx, stream, void)
          else
            stream.push(nil) if (!void)
          end
          stream.set_label(done_label)
        end
      end

      class ScriptNode < Node
        attr_accessor :body

        def compile(ctx, stream, void)
          stream.frame_set("script-exit")
          stream.pop

          body.compile_node(ctx, stream, false)

          stream.frame_get("script-exit")
          stream.swap
          stream.channel_ret
        end
        def compile_node(ctx, stream)
          super(ctx, stream, false)
        end
      end

      rule(:nil => simple(:n)) { NilNode.new(n, :val => nil) }
      rule(:undef => simple(:s)) { UndefNode.new(s, :val => Channel9::Primitive::Undef) }
      rule(:true => simple(:s)) { TrueNode.new(s, :val => true) }
      rule(:false => simple(:s)) { FalseNode.new(s, :val => false) }
      rule(:integer => simple(:i)) { IntegerConstantNode.new(i, :val => i.to_i) }
      rule(:string => simple(:s)) { StringConstantNode.new(s, :val => s.to_s) }
      rule(:list => sequence(:items)) { ListNode.new(items[0], :items => items) }
      rule(:list => simple(:item)) { ListNode.new(item, :items => [item]) }

      rule(:message_id => simple(:n), :string => simple(:name)) { MessageIdNode.new(n, :val => Primitive::MessageID.new(name.to_s)) }
      rule(:protocol_id => simple(:n), :string => simple(:name)) { ProtocolIdNode.new(n, :val => Primitive::ProtocolID.new(name.to_s)) }

      rule(:argdef => simple(:arg)) { arg.nil? ? [] : [arg] }
      rule(:argdef => sequence(:args)) { args }
      rule(:func => simple(:body), :args => sequence(:args)) {
        FunctionNode.new(body, :body => body, :args => args)
      }
      rule(:func => simple(:body), :args => sequence(:args), :output_var => simple(:output)) {
        FunctionNode.new(body, :body => body, :args => args, :output => output)
      }
      rule(:func => simple(:body), :args => sequence(:args), :output_var => simple(:output), :msg_var => simple(:msg)) {
        FunctionNode.new(body, :body => body, :args => args, :output => output, :msg => msg)
      }
      rule(:func => simple(:body), :args => sequence(:args), :msg_var => simple(:msg)) {
        FunctionNode.new(body, :body => body, :args => args, :msg => msg)
      }
      rule(:func => simple(:body), :args => sequence(:args)) {
        FunctionNode.new(body, :body => body, :args => args)
      }
      rule(:func => simple(:body), :args => simple(:args), :output_var => simple(:output)) {
        FunctionNode.new(body, :body => body, :args => [args], :output => output)
      }
      rule(:func => simple(:body), :args => simple(:args)) {
        FunctionNode.new(body, :body => body, :args => [args])
      }
      rule(:func => simple(:body), :args => simple(:args), :output_var => simple(:output), :msg_var => simple(:msg)) {
        FunctionNode.new(body, :body => body, :args => [args], :output => output, :msg => msg)
      }
      rule(:func => simple(:body), :args => simple(:args), :msg_var => simple(:msg)) {
        FunctionNode.new(body, :body => body, :args => [args], :msg => msg)
      }
      rule(:func => simple(:body), :output_var => simple(:output), :msg_var => simple(:msg)) {
        FunctionNode.new(body, :body => body, :output => output, :msg => msg)
      }
      rule(:func => simple(:body), :msg_var => simple(:msg)) {
        FunctionNode.new(body, :body => body, :msg => msg)
      }

      rule(:local_var => simple(:name)) { LocalVariableNode.new(name, :name => name.to_s) }
      rule(:declare_var => simple(:name)) { LocalDeclareNode.new(name, :name => name.to_s, :type => "local") }
      rule(:declare_var => simple(:name), :type => simple(:type)) { LocalDeclareNode.new(name, :name => name.to_s, :type => type.to_s) }
      rule(:declare_var => simple(:name), :type => simple(:type), :assign => simple(:expression)) { 
        LocalDeclareNode.new(name, :name => name.to_s, :type => type.to_s, :expression => expression) 
      }
      rule(:special_var => simple(:name)) { SpecialVariableNode.new(name, :name => name.to_s) }

      rule(:expr => simple(:expr)) { expr }
      rule(:send => simple(:expression), :target => simple(:target)) { 
        SendNode.new(expression, :target => target, :expression => expression) 
      }
      rule(:send => simple(:expression), :target => simple(:target), :continuation => simple(:continuation)) { 
        SendNode.new(expression, :target => target, :expression => expression, :continuation => continuation) 
      }
      rule(:return => simple(:expression), :target => simple(:target)) { 
        ReturnNode.new(expression, :target => target, :expression => expression) 
      }

      rule(:text => simple(:text)) { text }
      rule(:inline_doc => simple(:text)) { CommentNode.new(text, :text => text) }

      rule(:statement => {:doc => sequence(:doc), :expr => simple(:expr)}) { 
        StatementNode.new(expr, :doc => doc.join("\n"), :expression => expr) 
      }
      rule(:sequence => sequence(:statements)) {
        SequenceNode.new(statements[0], :statements => statements)
      }
      rule(:sequence => simple(:statement)) {
        SequenceNode.new(statement, :statements => statement.nil? ? [] : [statement])
      }

      rule(:args => sequence(:args)) { args }
      rule(:args => simple(:args)) { [args] }
      rule(:value_invoke => sequence(:args)) { ValueInvokeNode.new(nil, :args => args) }
      rule(:value_invoke => simple(:a)) { ValueInvokeNode.new(nil, :args => []) }
      rule(:member_invoke => {:name => simple(:name), :args => sequence(:args)}) {
        MemberInvokeNode.new(name, :name => name.to_s, :args => args)
      }
      rule(:member_invoke => {:name => simple(:name), :args => simple(:arg)}) {
        MemberInvokeNode.new(name, :name => name.to_s, :args => [arg])
      }
      rule(:member_invoke => {:name => simple(:name)}) { MemberInvokeNode.new(name, :name => name.to_s, :args => []) }
      rule(:call => sequence(:call), :on => simple(:on)) {
        res = on
        call.each {|action|
          res = CallNode.new(res, :on => res, :action => action)
        }
        res
      }

      rule(:instruction => simple(:name), :args => sequence(:args)) {
        InstructionNode.new(name, :name => name.to_sym, :args => args)
      }

      rule(:bytecode => sequence(:insns), :input => simple(:input)) {
        BytecodeNode.new(insns[0], :instructions => insns, :inputs => [input])
      }
      rule(:bytecode => sequence(:insns), :input => sequence(:input)) {
        BytecodeNode.new(insns[0], :instructions => insns, :inputs => input)
      }

      rule(:assign_to => simple(:var), :op => simple(:op)) { AssignTargetNode.new(op, :var => var, :op => op.to_s) }
      rule(:assign => simple(:expr), :left => sequence(:left)) {
        res = expr
        left.each {|asgn|
          res = AssignmentNode.new(asgn, :to => asgn, :expr => res)
        }
        res
      }

      rule(:op => simple(:op), :right => simple(:right)) { OperatorNode.new(op, :op => op.to_s, :right => right) }

      {
        :sum => SumNode,
        :product => ProductNode,
        :equality => EqualityNode,
        :relational => RelationalNode,
      }.each do |type, node|
        rule(type => sequence(:expr), :left => simple(:on)) {
          res = on
          expr.each {|action|
            res = node.new(res, :on => res, :action => action)
          }
          res
        }
      end

      rule(:if => simple(:cond), :block => simple(:block)) {
        IfNode.new(cond, :condition => cond, :block => block)
      }
      rule(:if => simple(:cond), :block => simple(:block), :else => simple(:e)) {
        IfNode.new(cond, :condition => cond, :block => block, :else => e)
      }

      rule(:while => simple(:cond), :block => simple(:block)) {
        WhileNode.new(cond, :condition => cond, :block => block)
      }

      rule(:case => simple(:value), :block => simple(:block)) {
        CaseNode.new(value, :value => value, :block => block)
      }
      rule(:cases => sequence(:cases)) { cases }
      rule(:cases => simple(:cases)) { [cases] }

      rule(:switch => simple(:cond), :cases => sequence(:cases)) {
        SwitchNode.new(cond, :condition => cond, :cases => cases, :else => nil)
      }
      rule(:switch => simple(:cond), :cases => sequence(:cases), :else => simple(:e)) {
        SwitchNode.new(cond, :condition => cond, :cases => cases, :else => e)
      }
      
      rule(:script => simple(:body)) { ScriptNode.new(body, :body => body) }
    end
  end
end