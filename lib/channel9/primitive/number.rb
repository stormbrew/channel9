module Channel9
  module Primitive
    class Number < Base
      attr :real_num
      def initialize(real_num)
        @real_num = real_num
      end
      def self.coerce(other)
        if (other.kind_of? Base)
          other = other.c9_to_num
        else
          raise "Unimplemented: turning arbitrary values into numbers"
        end
        raise "Primitive error: Not a number" if (!other.kind_of? Number)
        other  
      end        

      def ==(other)
        other.is_a?(Number) && @real_num == other.real_num
      end
      alias_method :eql?, :==
      def hash
        @real_num.hash
      end

      def to_s
        @real_num.to_s
      end
      def to_json(*a)
        @real_num.to_json(*a)
      end

      def c9_to_num
        self
      end

      def c9_plus(other)
        return @real_num + Number.coerce(other).real_num
      end
      alias_method :"c9_+", :c9_plus
      def c9_minus(other)
        return @real_num - Number.coerce(other).real_num
      end
      alias_method :"c9_minus", :c9_minus
      def c9_mul(other)
        return @real_num * Number.coerce(other).real_num
      end
      alias_method :"c9_*", :c9_mul
      def c9_div(other)
        return @real_num / Number.coerce(other).real_num
      end
      alias_method :"c9_/", :c9_div
      def c9_mod(other)
        return @real_num % Number.coerce(other).real_num
      end
      alias_method :"c9_%", :c9_mod

      def c9_lt(other)
        return @real_num < Number.coerce(other).real_num
      end
      alias_method :"c9_<", :c9_lt
      def c9_gt(other)
        return @real_num > Number.coerce(other).real_num
      end
      alias_method :"c9_>", :c9_gt
      def c9_equal(other)
        return @real_num == Number.coerce(other).real_num
      end
      alias_method :"c9_==", :c9_equal
      def c9_notequal(other)
        return @real_num != Number.coerce(other).real_num
      end
      alias_method :"c9_!=", :c9_notequal
    end
  end
end

class Fixnum
  def to_c9
    Channel9::Primitive::Number.new(self)
  end
end

class Bignum
  def to_c9
    Channel9::Primitive::Number.new(self)
  end
end