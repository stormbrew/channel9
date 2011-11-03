module Channel9
  def self.setup_environment(rb_exe, argv)
    Object.const_set(:RUBY_EXE, rb_exe.to_s)
    # argv comes in as a tuple of symbols.
    Object.const_set(:ARGV, argv.collect {|i| i.to_s }.to_a)

    Object.const_set(:ENV, {}) # TODO: Make this not a stub.

    $stderr = Stderr.new
    $stdout = $> = Stdout.new
    $stdin = $< = Stdout.new
  end

  def self.compile_string(type, string, filename, line)
    compiled = $__c9_loader.compile(type.to_s_prim, string.to_s_prim, filename.to_s_prim, line.to_i)
    if (compiled) # never ever do more than test the compiled object or it will invoke the closure.
      return Proc.new_from_prim(compiled)
    else
      nil
    end
  end
end
# These are targets for what we're trying to be compatible
# with.
RUBY_PLATFORM = "i686-darwin10.3.0"
RUBY_VERSION = "1.8.7"
RUBY_PATCHLEVEL = "334"
# Commented out to make rubyspec happy (for now)
#RUBY_ENGINE = "c9.rb"
