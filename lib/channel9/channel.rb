module Channel9
  class Channel
    attr :context
    attr :position

    def initialize(context, position)
      @context = context.dup
      @position = position
    end

    def call(environment, val, ret)
      new_context = @context.dup
      new_context.start(@position, [val, ret])
      environment.set_context(new_context)
    end
  end
end