module Channel9
  module Primitive
    class Base
      def channel_send(val, ret)
        if (val.kind_of? Message)
          result = self.send(:"c9_#{val.name}", *val.positional)
          ret.channel_send(result.to_c9, InvalidReturnChannel)
        else
          raise "Primitive method error: #{val}"
        end
      end

      def to_c9
        self
      end

      def truthy?
        true
      end
    end
  end
end

require 'channel9/primitive/boolean'
require 'channel9/primitive/nil'
require 'channel9/primitive/undef'
require 'channel9/primitive/message'
require 'channel9/primitive/number'
require 'channel9/primitive/string'
require 'channel9/primitive/tuple'
require 'channel9/primitive/table'