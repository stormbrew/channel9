module Channel9
  module Primitive
    class Tuple < Base
      attr :real_ary
      def initialize(ary)
        @real_ary = ary.dup.freeze
      end

      def to_s
        @real_ary.to_s
      end
      def to_json(*a)
        @real_ary.to_json(*a)
      end

      def ==(other)
        other.is_a?(Tuple) && @real_ary == other.real_ary
      end
    end
  end
end

class Array
  def to_c9
    Channel9::Primitive::Tuple.new(self)
  end
end