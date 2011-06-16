module Channel9
  module Primitive
    class Base
      def channel_send(cenv, val, ret)
        if (val.kind_of?(Message))
          if (self.respond_to?(:"c9_#{val.name}"))
            result = self.send(:"c9_#{val.name}", *val.positional)
            ret.channel_send(cenv, result.to_c9, InvalidReturnChannel)
            return
          else
            if (ext = cenv.special_channel[self.class.name])
              msg = val.prefix(self).forward(:__c9_primitive_call__)
              ext.channel_send(cenv, msg, ret)
              return
            end
          end
        end
        raise "Primitive method error: #{val}"
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