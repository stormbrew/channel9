# Convert the $LOAD_PATH to a normal array instead of the
# tuple it started as.
$LOAD_PATH = Array.new($__c9_initial_load_path)
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

load 'delta/kernel.rb'
load 'delta/object.rb'
load 'delta/class.rb'
load 'delta/module.rb'

load 'delta/comparable.rb'

load 'delta/channel9.rb'

load 'delta/string.rb'
load 'delta/fixnum.rb'
load 'delta/float.rb'
load 'delta/message.rb'
load 'delta/table.rb'
load 'delta/range.rb'
load 'delta/hash.rb'

load 'delta/file.rb'
load 'delta/dir.rb'
load 'delta/time.rb'

load 'delta/regexp.rb'

load 'delta/signal.rb'
load 'delta/thread.rb'