class Exception
  def initialize(msg)
    if (msg.nil?)
      @msg = self.class.name
    else
      @msg = msg
    end
  end

  def self.exception(msg)
    new(msg)
  end
  def exception(msg)
    if (@msg == msg || msg.nil?)
      self
    else
      self.class.new(msg)
    end
  end

  def backtrace
    @backtrace
  end
  def set_backtrace(bt)
    @backtrace = bt
  end

  def to_s
    @msg
  end
  def to_str
    @msg
  end
  def message
    to_s
  end
end

class StandardError < Exception; end

class RuntimeError < StandardError; end