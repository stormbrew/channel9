module Channel9
  module Primitive
    class UndefC
      def to_s
        "<undef>"
      end
      def to_json(*a)
        {"undef"=>"undef"}.to_json(*a)
      end
    end
    Undef = UndefC.new
  end
end
