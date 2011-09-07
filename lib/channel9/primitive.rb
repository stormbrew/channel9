module Channel9
  module Primitive
    class Base
      def channel_send(cenv, val, ret)
        if (val.kind_of?(Message))
          if (self.respond_to?(:"c9_#{val.name}"))
            begin
              result = self.send(:"c9_#{val.name}", *val.positional)
              ret.channel_send(cenv, result, InvalidReturnChannel)
              return
            rescue
              # let the environment's singleton class deal with it below
            end
          end
        end
        pp self.class.name
        if (ext = cenv.special_channel[self.class.name])
          msg = val.prefix(self).forward(:__c9_primitive_call__)
          ext.channel_send(cenv, msg, ret)
          return
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
require 'channel9/primitive/float'
require 'channel9/primitive/string'
require 'channel9/primitive/tuple'
require 'channel9/primitive/table'