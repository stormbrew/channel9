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
      class ListConstantNode < ConstantNode; end

      class FunctionNode < Node
        attr_accessor :args, :output, :body

        def compile(ctx, stream)
          prefix = ctx.label_prefix('func')
          done_label = prefix + "done"
          body_label = prefix + "body"

          stream.jmp(done_label)
          stream.set_label(body_label)
          # compile body here
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
          raise "Undeclared variable '#{var.name}' at #{ctx.filename}:#{line}:#{col}" if !var_depth
          stream.local_get(var_depth, name)
        end
      end
      class LocalDeclareNode < LocalVariableNode
        def find(ctx)
          ctx.declare_var(name)
          0
        end
      end

      class CallActionNode < Node; end
      class ValueInvokeNode < CallActionNode
        attr_accessor :args

        def compile(ctx, stream)
          args.reverse.each do |arg|
            arg.compile_node(ctx, stream)
          end
          stream.message_new("call", 0, args.length)
        end
      end
      class MemberInvokeNode < CallActionNode
        attr_accessor :name, :args
      end

      class CallNode < Node
        attr_accessor :on, :action

        def compile(ctx, stream)
          on.compile(ctx, stream)
          action.compile_node(ctx, stream)
          stream.channel_call
          stream.pop # get rid of unwanted return channel
        end
      end

      class SendNode < Node
        attr_accessor :target, :expression

        def compile(ctx, stream)
          # Todo: Make this recognize tail recursion.
          target.compile_node(ctx, stream)
          expression.compile_node(ctx, stream)
          stream.channel_ret
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
          statements.each_with_index do |stmt, idx|
            stmt.compile_node(ctx, stream)
            # only leave the last result on the stack.
            stream.pop if (idx != statements.length - 1)
          end
        end
      end

      class AssignTargetNode < Node
        attr_accessor :var, :op

        def compile(ctx, stream)
          raise "Unsupported assignment operator" if op != "="
          var_depth = var.find(ctx)
          raise "Undeclared variable '#{var.name}' at #{ctx.filename}:#{line}:#{col}" if !var_depth

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
      end
      class BinOpNode < Node
        attr_accessor :on, :action
      end
      class SumNode < BinOpNode; end
      class ProductNode < BinOpNode; end
      class EqualityNode < BinOpNode; end

      class IfNode < Node
        attr_accessor :condition, :block, :else
      end

      rule(:nil => simple(:n)) { NilNode.new(n, :val => nil) }
      rule(:integer => simple(:i)) { IntegerConstantNode.new(i, :val => i.to_i) }
      rule(:string => simple(:s)) { StringConstantNode.new(s, :val => s.to_s) }

      rule(:argdef => simple(:arg)) { arg.nil? ? [] : [arg] }
      rule(:argdef => sequence(:args)) { args }
      rule(:func => simple(:body), :args => sequence(:args)) {
        FunctionNode.new(body, :body => body, :args => args)
      }
      rule(:func => simple(:body), :args => sequence(:args), :output_var => simple(:output)) {
        FunctionNode.new(body, :body => body, :args => args, :output => output)
      }

      rule(:local_var => simple(:name)) { LocalVariableNode.new(name, :name => name.to_s) }
      rule(:declare_var => simple(:name)) { LocalDeclareNode.new(name, :name => name.to_s) }
      rule(:special_var => simple(:name)) { SpecialVariableNode.new(name, :name => name.to_s) }

      rule(:expr => simple(:expr)) { expr }
      rule(:send => sequence(:info)) { SendNode.new(info[0], :target => info[0], :expression => info[1]) }

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
      rule(:value_invoke => sequence(:args)) { ValueInvokeNode.new(nil, :args => args) }
      rule(:value_invoke => simple(:a)) { ValueInvokeNode.new(nil, :args => []) }
      rule(:member_invoke => {:name => simple(:name), :args => sequence(:args)}) {
        MemberInvokeNode.new(name, :name => name, :args => args)
      }
      rule(:member_invoke => {:name => simple(:name)}) { MemberInvokeNode.new(name, :name => name) }
      rule(:call => sequence(:call), :on => simple(:on)) {
        res = on
        call.each {|action|
          res = CallNode.new(res, :on => res, :action => action)
        }
        res
      }

      rule(:assign_to => simple(:var), :op => simple(:op)) { AssignTargetNode.new(op, :var => var, :op => op) }
      rule(:assign => simple(:expr), :left => sequence(:left)) {
        res = expr
        left.each {|asgn|
          res = AssignmentNode.new(asgn, :to => asgn, :expr => res)
        }
        res
      }

      rule(:op => simple(:op), :right => simple(:right)) { OperatorNode.new(op, :op => op, :right => right) }

      {
        :sum => SumNode,
        :product => ProductNode,
        :equality => EqualityNode,
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

    end
  end
end