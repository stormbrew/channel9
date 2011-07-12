module Channel9
  module Primitive
    class Boolean < Base; end
    class TrueC < Boolean
      def to_s
        "true"
      end
      def to_json(*a)
        true.to_json(*a)
      end
    end
    class FalseC < Boolean
      def to_s
        "false"
      end
      def to_json(*a)
        false.to_json(*a)
      end
      def truthy?
        false
      end
    end
    True = TrueC.new.freeze
    False = FalseC.new.freeze
  end
end

class TrueClass
  def to_c9
    self
  end
  def self.channel_name; :"Channel9::Primitive::TrueC"; end
end
class FalseClass
  def to_c9
    self
  end
  def self.channel_name; :"Channel9::Primitive::FalseC"; end
end