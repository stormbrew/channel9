module Channel9
  module Primitive
    class NilC < Base
      def to_s
        "nil"
      end
      def truthy?
        false
      end
      def to_json(*a)
        nil.to_json(*a)
      end
      def nil?
        true
      end
    end
    Nil = NilC.new.freeze
  end
end

class NilClass
  def to_c9
    self
  end
  def self.channel_name; :"Channel9::Primitive::NilC"; end
end