module Channel9
  module Primitive
    class String < Base
      attr :real_str
      def initialize(str)
        @real_str = str.dup.intern
      end

      def self.coerce(other)
        if (other.kind_of? Base)
          other = other.c9_to_str
        else
          raise "Unimplemented: turning arbitrary values into strings"
        end
        raise "Primitive error: Not a string" if (!other.kind_of? String)
        other  
      end        

      def ==(other)
        other.is_a?(String) && @real_str == other.real_str
      end
      alias_method :eql?, :==
      def hash
        @real_str.hash
      end

      def to_s
        @real_str.to_s
      end
      def to_json(*a)
        @real_str.to_json(*a)
      end
      
      def c9_to_str
        self
      end

      def c9_plus(other)
        return @real_str + String.coerce(other).real_str
      end
    end
  end
end

class String
  def to_c9
    Channel9::Primitive::String.new(self)
  end
end
class Symbol
  def to_c9
    Channel9::Primitive::String.new(self.to_s)
  end
end