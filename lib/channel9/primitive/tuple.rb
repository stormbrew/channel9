module Channel9
  module Primitive
    class Tuple < Base
      attr :real_ary
      def initialize(ary)
        @real_ary = ary.dup.freeze
      end

      def to_s
        "#<Tuple:[#{@real_ary.join(",")}]>"
      end
      def to_json(*a)
        @real_ary.to_json(*a)
      end

      def ==(other)
        other.is_a?(Tuple) && @real_ary == other.real_ary
      end
      alias_method :eql?, :==

      def c9_length
        @real_ary.length
      end
      def c9_join(other)
        Tuple.new(@real_ary + other.real_ary)
      end
      alias_method :'c9_+', :c9_join
      def c9_push(other)
        Tuple.new(@real_ary + [other])
      end
      def c9_pop
        n = @real_ary.dup
        n.pop
        Tuple.new(n)
      end
      def c9_front_push(other)
        Tuple.new([other] + @real_ary)
      end
      def c9_front_pop
        n = @real_ary.dup
        n.shift
        Tuple.new(n)
      end
      def c9_delete(i)
        n = @real_ary.dup
        n.delete_at(i)
        Tuple.new(n)
      end
      def c9_first
        @real_ary.first
      end
      def c9_last
        @real_ary.last
      end
      def c9_at(idx)
        @real_ary[idx.real_num].to_c9
      end
      def c9_subary(first, last)
        Tuple.new(@real_ary[first.real_num...last.real_num])
      end
      def c9_replace(idx, val)
        n = @real_ary.dup
        n[idx.real_num] = val
        Tuple.new(n)
      end
      def hash
        @real_ary.hash
      end
    end
  end
end

class Array
  def to_c9
    self
  end
end