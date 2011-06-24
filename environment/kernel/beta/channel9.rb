module Channel9
  def self.setup_environment(argv)
    # argv comes in as a tuple of symbols.
    Object.const_set(:ARGV, argv.collect {|i| i.to_s }.to_a)

    Object.const_set(:ENV, {}) # TODO: Make this not a stub.
  end
end
# These are targets for what we're trying to be compatible
# with.
RUBY_PLATFORM = "i686-darwin10.3.0"
RUBY_VERSION = "1.8.7"
RUBY_PATCHLEVEL = "334"
