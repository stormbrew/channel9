module Channel9
  module Primitive
    class String < Base
      attr :real_str
      def initialize(str)
        if (str.is_a?(String))
          @real_str = str.real_str
        else
          @real_str = str.dup.freeze
        end
      end

      def self.coerce(other)
        if (other.kind_of? Base)
          other = other.to_c9_str
        else
          raise "Unimplemented: turning arbitrary values into strings"
        end
        raise "Primitive error: Not a string" if (!other.kind_of? String)
        other  
      end

      def ==(other)
        other.is_a?(String) && @real_str == other.real_str ||
        other.is_a?(Symbol) && @real_str == other.to_s
      end
      alias_method :eql?, :==
      alias_method :"c9_==", :==
      def hash
        @real_str.hash
      end

      def to_s
        @real_str.to_s
      end
      def to_c9_str
        self
      end
      def to_sym
        @real_str.to_sym
      end
      def to_json(*a)
        @real_str.to_json(*a)
      end
      
      def c9_plus(other)
        return String.new(@real_str + String.coerce(other).real_str)
      end
      alias_method :"c9_+", :c9_plus

      def c9_split(by)
        by = String.coerce(by).real_str
        @real_str.split(by).collect {|s| String.new(s) }.to_c9
      end
      def c9_substr(first, last)
        String.new(@real_str[first.real_num..last.real_num])
      end
    end
  end
end

# Note: C9 uses immutable strings, so it doesn't make
# sense to provide to_c9 for String, since such a thing
# would probably interfere with langauge's attempts to
# provide a mutable string object.
class Symbol
  def to_c9
    Channel9::Primitive::String.new(self.to_s)
  end
end