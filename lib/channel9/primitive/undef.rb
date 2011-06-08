module Channel9
  module Primitive
    class UndefC < Base
      def truthy?
        false
      end

      def to_s
        "<undef>"
      end
      def to_json(*a)
        [null, "undef"].to_json(*a)
      end
    end
    Undef = UndefC.new.freeze
  end
end

module Kernel
  def undef_c9
    Channel9::Primitive::Undef
  end
end