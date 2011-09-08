module Channel9
  module Script
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
    end

    class ConstantNode < Node
      attr_accessor :val
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
    end

    class VariableNode < Node
      attr_accessor :name
    end
    class LocalVariableNode < VariableNode; end
    class SpecialVariableNode < VariableNode; end

    class CallActionNode < Node; end
    class ValueInvokeNode < CallActionNode
      attr_accessor :args
    end
    class MemberInvokeNode < CallActionNode
      attr_accessor :name, :args
    end

    class CallNode < Node
      attr_accessor :on, :action
    end

    class SendNode < Node
      attr_accessor :target, :expression
    end

    class StatementNode < Node
      attr_accessor :doc, :expression
    end
    class SequenceNode < Node
      attr_accessor :statements
    end

    class AssignTargetNode < Node
      attr_accessor :var, :op
    end
    class AssignmentNode < Node
      attr_accessor :to, :expr
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

    class Transformer < Parslet::Transform
      rule(:nil => simple(:n)) { NilNode.new(n) }
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