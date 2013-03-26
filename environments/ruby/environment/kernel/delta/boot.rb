# Convert the $LOAD_PATH to a normal array instead of the
# tuple it started as.
$LOAD_PATH = Array.new_from_tuple($__c9_initial_load_path)
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

$0 = $__c9_argv.at(0)

if !$0
  puts "You didn't specify a script to load."
  exit(1)
end

Channel9.setup_environment('c9', $__c9_argv.front_pop)

def include(mod)
  __c9_make_singleton__.include(mod)
end

if $__c9_trace_loaded
  __c9_debugger__ :trace_enable
end
load $0
