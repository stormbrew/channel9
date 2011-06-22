# Convert the $LOAD_PATH to a normal array instead of the
# tuple it started as.
$LOAD_PATH = Array.new($LOAD_PATH)
$: = $LOAD_PATH

load 'beta/object.rb'
load 'beta/class.rb'
load 'beta/module.rb'

load 'beta/fixnum.rb'
load 'beta/message.rb'
load 'beta/table.rb'

load 'beta/hash.rb'

load 'beta/file.rb'
