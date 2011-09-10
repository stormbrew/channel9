module Channel9
  module Script
    class Transformer < Parslet::Transform
      class Context
        attr :filename
        attr :last_line, true

        def initialize(filename)
          @filename = filename
          @label_prefixes = Hash.new {|h,k| h[k] = 0 }
          @variable_frames = [{}]
        end

        def label_prefix(name)
          num = @label_prefixes[name] += 1
          name + "." + num.to_s + ":"
        end

        def find_var(name)
          @variable_frames.each_with_index do |frame, idx|
            return idx if frame[name]
          end
          nil
        end
        def variable_frame(&block)
          begin
            @variable_frames.unshift({})
            block.call
          ensure
            @variable_frames.shift
          end
        end
        def declare_var(name)
          @variable_frames.first[name] = true
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
        end
        def line_and_column
          [@line, @col]
        end

        def compile_node(ctx, stream)
          line_info = [ctx.filename, line, col]
          if (ctx.last_line != line_info && line && col)
            stream.line(*line_info)
            ctx.last_line = line_info
          end
          compile(ctx, stream)
        end
      end

      class CommentNode < Node
        attr_accessor :text

        def compile(ctx, stream)
        end
      end

      class ConstantNode < Node
        attr_accessor :val

        def compile(ctx, stream)
          stream.push(val)
        end
      end
      class NilNode < ConstantNode; end
      class UndefNode < ConstantNode; end
      class TrueNode < ConstantNode; end
      class FalseNode < ConstantNode; end
      class IntegerConstantNode < ConstantNode; end
      class StringConstantNode < ConstantNode; end
      
      class ListNode < Node
        attr_accessor :items

        def compile(ctx, stream)
          if (items.length == 1 && !items.first.is_a?(Node))
            stream.tuple_new(0)
          else
            items.reverse.each do |item|
              item.compile_node(ctx, stream)
            end
            stream.tuple_new(items.length)
          end
        end
      end

      class FunctionNode < Node
        attr_accessor :args, :msg, :output, :body

        def compile(ctx, stream)
          prefix = ctx.label_prefix('func')
          done_label = prefix + "done"
          body_label = prefix + "body"

          stream.jmp(done_label)
          stream.set_label(body_label)

          stream.local_linked_scope
          ctx.variable_frame do
            if (output)
              stream.dup_top
              ctx.declare_var(output.name)
              stream.local_set(0, output.name)
            end  
            stream.frame_set("return")

            if (args && args.first && args.length > 0)
              stream.message_unpack(args.length, 0, 0)

              args.each do |arg|
                ctx.declare_var(arg.name)
                stream.local_set(0, arg.name)
              end
            end

            if (msg.nil?)
              stream.pop # remove the message
            else
              ctx.declare_var(msg)
              stream.local_set(0, msg)
            end

            body.compile_node(ctx, stream)

            stream.frame_get("return")
            stream.swap
            stream.channel_ret
          end

          stream.set_label(done_label)
          stream.channel_new(body_label)
        end
      end

      class VariableNode < Node
        attr_accessor :name
      end
      class SpecialVariableNode < VariableNode
        def compile(ctx, stream)
          stream.channel_special(name)
        end
      end
      class LocalVariableNode < VariableNode 
        def find(ctx)
          ctx.find_var(name)
        end

        def compile(ctx, stream)
          var_depth = find(ctx)
          raise "Undeclared variable '#{name}' at #{ctx.filename}:#{line}:#{col}" if !var_depth
          stream.local_get(var_depth, name)
        end
      end
      class LocalDeclareNode < LocalVariableNode
        attr_accessor :expression

        def find(ctx)
          ctx.declare_var(name)
          0
        end

        def compile(ctx, stream)
          if (expression.nil?)
            super
          else
            find(ctx)
            expression.compile_node(ctx, stream)
            stream.dup_top
            stream.local_set(0, name)
          end
        end
      end

      class CallActionNode < Node; end
      class ValueInvokeNode < CallActionNode
        attr_accessor :args

        def compile(ctx, stream)
          args.each do |arg|
            arg.compile_node(ctx, stream)
          end
          stream.message_new("call", 0, args.length)
        end
      end
      class MemberInvokeNode < CallActionNode
        attr_accessor :name, :args

        def compile(ctx, stream)
          args.each do |arg|
            arg.compile_node(ctx, stream)
          end
          stream.message_new(name, 0, args.length)
        end
      end

      class CallNode < Node
        attr_accessor :on, :action

        def compile(ctx, stream)
          on.compile_node(ctx, stream)
          action.compile_node(ctx, stream)
          stream.channel_call
          stream.pop # get rid of unwanted return channel
        end
      end
      class SendNode < Node
        attr_accessor :target, :expression, :continuation

        def compile(ctx, stream)
          # Todo: Make this recognize tail recursion.
          target.compile_node(ctx, stream)
          expression.compile_node(ctx, stream)
          stream.channel_call
          if (continuation.nil?)
            stream.pop
          else
            depth = continuation.find(ctx)
            stream.local_set(depth, continuation.name)
          end
        end
      end
      class ReturnNode < Node
        attr_accessor :target, :expression

        def compile(ctx, stream)
          # TODO: Make this not have such intimate knowledge
          # of CallNode and SendNode
          case expression
          when CallNode
            expression.on.compile_node(ctx, stream)
            target.compile_node(ctx, stream)
            expression.action.compile_node(ctx, stream)
            stream.channel_send
          else
            target.compile_node(ctx, stream)
            expression.compile_node(ctx, stream)
            stream.channel_ret
          end
        end
      end

      class StatementNode < Node
        attr_accessor :doc, :expression

        def compile(ctx, stream)
          expression.compile_node(ctx, stream)
        end
      end
      class SequenceNode < Node
        attr_accessor :statements

        def compile(ctx, stream)
          if (statements.length == 1 && !statements.first.is_a?(Node))
            stream.push(nil)
          else
            statements.each_with_index do |stmt, idx|
              stmt.compile_node(ctx, stream)
              # only leave the last result on the stack.
              stream.pop if (idx != statements.length - 1)
            end
          end
        end
      end

      class AssignTargetNode < Node
        attr_accessor :var, :op

        def compile(ctx, stream)
          var_depth = var.find(ctx)
          raise "Undeclared variable '#{var.name}' at #{ctx.filename}:#{line}:#{col}" if !var_depth

          if (op != '=')
            real_op = op.gsub(/=$/, '')
            stream.message_new(real_op, 0, 1)
            stream.local_get(var_depth, var.name)
            stream.swap
            stream.channel_call
            stream.pop
          end

          stream.dup_top
          stream.local_set(var_depth, var.name)
        end
      end
      class AssignmentNode < Node
        attr_accessor :to, :expr

        def compile(ctx, stream)
          expr.compile_node(ctx, stream)
          to.compile_node(ctx, stream)
        end
      end

      class OperatorNode < Node
        attr_accessor :op, :right

        def compile(ctx, stream)
          right.compile_node(ctx, stream)
          stream.message_new(op, 0, 1)
        end
      end
      class BinOpNode < Node
        attr_accessor :on, :action

        def compile(ctx, stream)
          on.compile_node(ctx, stream)
          action.compile_node(ctx, stream)
          stream.channel_call
          stream.pop
        end
      end
      class SumNode < BinOpNode; end
      class ProductNode < BinOpNode; end
      class RelationalNode < BinOpNode; end
      class EqualityNode < BinOpNode
        def compile(ctx, stream)
          case action.op
          when '=='
            on.compile_node(ctx, stream)
            action.right.compile_node(ctx, stream)
            stream.is_eq
          when '!='
            on.compile_node(ctx, stream)
            action.right.compile_node(ctx, stream)
            stream.is_not_eq
          else
            super
          end
        end
      end

      class IfNode < Node
        attr_accessor :condition, :block, :else

        def compile(ctx, stream)
          prefix = ctx.label_prefix('if')
          done_label = prefix + "done"
          else_label = prefix + "else"

          condition.compile_node(ctx, stream)
          stream.jmp_if_not(else_label)
          
          block.compile_node(ctx, stream)
          stream.jmp(done_label)
          
          stream.set_label(else_label)
          if (!self.else.nil?)
            self.else.compile_node(ctx, stream)
          else
            stream.push(nil)
          end
          stream.set_label(done_label)
        end
      end

      class WhileNode < Node
        attr_accessor :condition, :block

        def compile(ctx, stream)
          prefix = ctx.label_prefix('while')
          start_label = prefix + "start"
          done_label = prefix + "done"

          stream.set_label(start_label)
          block.compile_node(ctx, stream)
          stream.pop
          condition.compile_node(ctx, stream)
          stream.jmp_if(start_label)
          stream.set_label(done_label)

          stream.push(nil)
        end
      end

      class ScriptNode < Node
        attr_accessor :body

        def compile(ctx, stream)
          stream.frame_set("script-exit")
          stream.pop

          body.compile_node(ctx, stream)

          stream.frame_get("script-exit")
          stream.swap
          stream.channel_ret
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

      rule(:argdef => simple(:arg)) { arg.nil? ? [] : [arg] }
      rule(:argdef => sequence(:args)) { args }
      rule(:func => simple(:body), :args => sequence(:args)) {
        FunctionNode.new(body, :body => body, :args => args)
      }
      rule(:func => simple(:body), :args => sequence(:args), :output_var => simple(:output)) {
        FunctionNode.new(body, :body => body, :args => args, :output => output)
      }
      rule(:func => simple(:body), :args => sequence(:args), :output_var => simple(:output), :msg_var => simple(:msg)) {
        FunctionNode.new(body, :body => body, :args => args, :output => output, :msg => msg.to_s)
      }
      rule(:func => simple(:body), :args => sequence(:args), :msg_var => simple(:msg)) {
        FunctionNode.new(body, :body => body, :args => args, :msg => msg.to_s)
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
        FunctionNode.new(body, :body => body, :args => [args], :output => output, :msg => msg.to_s)
      }
      rule(:func => simple(:body), :args => simple(:args), :msg_var => simple(:msg)) {
        FunctionNode.new(body, :body => body, :args => [args], :msg => msg.to_s)
      }
      rule(:func => simple(:body), :output_var => simple(:output), :msg_var => simple(:msg)) {
        FunctionNode.new(body, :body => body, :output => output, :msg => msg.to_s)
      }
      rule(:func => simple(:body), :msg_var => simple(:msg)) {
        FunctionNode.new(body, :body => body, :msg => msg.to_s)
      }

      rule(:local_var => simple(:name)) { LocalVariableNode.new(name, :name => name.to_s) }
      rule(:declare_var => simple(:name)) { LocalDeclareNode.new(name, :name => name.to_s) }
      rule(:declare_var => simple(:name), :assign => simple(:expression)) { 
        LocalDeclareNode.new(name, :name => name.to_s, :expression => expression) 
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
      
      rule(:script => simple(:body)) { ScriptNode.new(body, :body => body) }
    end
  end
end