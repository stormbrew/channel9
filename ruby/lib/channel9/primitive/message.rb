module Channel9
  module Primitive
    class MessageID
      def initialize(a)
        super(a.to_s)
      end

      def to_json(*a)
        {"message_id" => to_s}.to_json(*a)
      end
    end
    class ProtocolID
      def initialize(a)
        super(a.to_s)
      end

      def to_json(*a)
        {"protocol_id" => to_s}.to_json(*a)
      end
    end
  end
end
