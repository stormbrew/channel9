module Channel9
  module Primitive
    class Table < Base
      attr :real_hash
      def initialize(hash)
        @real_hash = hash.dup.freeze
      end

      def to_s
        @real_hash.to_s
      end

      def ==(other)
        other.is_a?(Table) && @real_hash == other.real_hash
      end
    end
  end
end

class Hash
  def to_c9
    Channel9::Primitive::Table.new(self)
  end
end