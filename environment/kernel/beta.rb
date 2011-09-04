# Convert the $LOAD_PATH to a normal array instead of the
# tuple it started as.
$LOAD_PATH = Array.new($LOAD_PATH)
$: = $LOAD_PATH
$LOADED_FEATURES = []

module Channel9
  def self.uncaught_exception(exc)
    puts("Uncaught Exception:", exc)
    exc.backtrace.each do |line|
      puts line
    end
  end
end

load 'beta/kernel.rb'
load 'beta/object.rb'
load 'beta/class.rb'
load 'beta/module.rb'

load 'beta/comparable.rb'

load 'beta/channel9.rb'

load 'beta/string.rb'
load 'beta/fixnum.rb'
load 'beta/float.rb'
load 'beta/message.rb'
load 'beta/table.rb'
load 'beta/range.rb'
load 'beta/hash.rb'

load 'beta/file.rb'
load 'beta/dir.rb'
load 'beta/time.rb'

load 'beta/regexp.rb'

load 'beta/signal.rb'
load 'beta/thread.rb'