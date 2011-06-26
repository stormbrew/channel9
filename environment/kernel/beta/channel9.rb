module Channel9
  def self.setup_environment(rb_exe, argv)
    Object.const_set(:RUBY_EXE, rb_exe.to_s)
    # argv comes in as a tuple of symbols.
    Object.const_set(:ARGV, argv.collect {|i| i.to_s }.to_a)

    Object.const_set(:ENV, {}) # TODO: Make this not a stub.

    $stderr = Object.new
    $stdout = Object.new
    $stdin = Object.new
  end
end
# These are targets for what we're trying to be compatible
# with.
RUBY_PLATFORM = "i686-darwin10.3.0"
RUBY_VERSION = "1.8.7"
RUBY_PATCHLEVEL = "334"
# Commented out to make rubyspec happy (for now)
#RUBY_ENGINE = "c9.rb"
