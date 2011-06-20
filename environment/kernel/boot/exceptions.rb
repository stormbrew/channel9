class Exception
  def initialize(msg = nil)
    if (msg.nil?)
      @msg = self.class.name
    else
      @msg = msg
    end
  end

  def self.exception(msg = nil)
    new(msg || @msg || name)
  end
  def exception(msg = nil)
    if (@msg == msg || msg.nil?)
      self
    else
      self.class.new(msg || @msg || self.class.name)
    end
  end

  def backtrace
    @backtrace
  end
  def set_backtrace(bt)
    @backtrace = bt
  end

  def to_s
    @msg.to_s
  end
  def to_str
    @msg.to_s
  end
  def message
    to_s
  end
end

class NoMemoryError < Exception; end

class ScriptError < Exception; end
class LoadError < ScriptError; end
class NotImplementedError < ScriptError; end
class SyntaxError < ScriptError; end

class SignalException < Exception; end
class Interrupt < Exception; end

class StandardError < Exception; end
class ArgumentError < StandardError; end
class IOError < StandardError; end
class EOFError < IOError; end
class IndexError < StandardError; end
class LocalJumpError < StandardError; end
class NameError < StandardError; end
class NoMethodError < NameError; end
class RangeError < StandardError; end
class FloatDomainError < RangeError; end
class RegexpError < StandardError; end
class RuntimeError < StandardError; end
class SecurityError < StandardError; end
class SystemCallError < StandardError; end
class SystemStackError < StandardError; end
class ThreadError < StandardError; end
class TypeError < StandardError; end
class ZeroDivisionError < StandardError; end

class SystemExit < Exception; end