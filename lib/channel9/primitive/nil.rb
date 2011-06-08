module Channel9
  module Primitive
    class NilC < Base
      def to_s
        nil.to_s
      end
      def truthy?
        false
      end
      def to_json(*a)
        nil.to_json(*a)
      end
    end
    Nil = NilC.new.freeze
  end
end

class NilClass
  def to_c9
    Channel9::Primitive::Nil
  end
end