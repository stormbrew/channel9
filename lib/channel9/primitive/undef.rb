module Channel9
  module Primitive
    class UndefC
      class <<self
        alias_method :channel_name, :name
      end

      def truthy?
        false
      end

      def to_s
        "<undef>"
      end
      def to_json(*a)
        [nil, "undef"].to_json(*a)
      end
    end
  end
end

module Kernel
  def undef_c9
    Channel9::Primitive::Undef
  end
end