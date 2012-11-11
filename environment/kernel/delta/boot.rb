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

load_c9 'kernel/delta/kernel.rb.c9b'
load_c9 'kernel/delta/object.rb.c9b'
load_c9 'kernel/delta/class.rb.c9b'
load_c9 'kernel/delta/module.rb.c9b'

load_c9 'kernel/delta/comparable.rb.c9b'

load_c9 'kernel/delta/channel9.rb.c9b'

load_c9 'kernel/delta/string.rb.c9b'
load_c9 'kernel/delta/fixnum.rb.c9b'
load_c9 'kernel/delta/float.rb.c9b'
load_c9 'kernel/delta/message.rb.c9b'
load_c9 'kernel/delta/table.rb.c9b'
load_c9 'kernel/delta/range.rb.c9b'
load_c9 'kernel/delta/hash.rb.c9b'

load_c9 'kernel/delta/io.rb.c9b'
load_c9 'kernel/delta/file.rb.c9b'
load_c9 'kernel/delta/dir.rb.c9b'
load_c9 'kernel/delta/time.rb.c9b'

load_c9 'kernel/delta/regexp.rb.c9b'

load_c9 'kernel/delta/signal.rb.c9b'
load_c9 'kernel/delta/thread.rb.c9b'
