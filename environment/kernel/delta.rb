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

load_c9 'kernel/delta/kernel.c9b'
load_c9 'kernel/delta/object.c9b'
load_c9 'kernel/delta/class.c9b'
load_c9 'kernel/delta/module.c9b'

load_c9 'kernel/delta/comparable.c9b'

load_c9 'kernel/delta/channel9.c9b'

load_c9 'kernel/delta/string.c9b'
load_c9 'kernel/delta/fixnum.c9b'
load_c9 'kernel/delta/float.c9b'
load_c9 'kernel/delta/message.c9b'
load_c9 'kernel/delta/table.c9b'
load_c9 'kernel/delta/range.c9b'
load_c9 'kernel/delta/hash.c9b'

load_c9 'kernel/delta/io.c9b'
load_c9 'kernel/delta/file.c9b'
load_c9 'kernel/delta/dir.c9b'
load_c9 'kernel/delta/time.c9b'

load_c9 'kernel/delta/regexp.c9b'

load_c9 'kernel/delta/signal.c9b'
load_c9 'kernel/delta/thread.c9b'