# Convert the $LOAD_PATH to a normal array instead of the
# tuple it started as.
$LOAD_PATH = Array.new($LOAD_PATH)
$: = $LOAD_PATH
$LOADED_FEATURES = []

load 'beta/kernel.rb'
load 'beta/object.rb'
load 'beta/class.rb'
load 'beta/module.rb'

load 'beta/fixnum.rb'
load 'beta/message.rb'
load 'beta/table.rb'

load 'beta/hash.rb'

load 'beta/file.rb'
load 'beta/dir.rb'

load 'beta/comparable.rb'

load 'beta/regexp.rb'

load 'beta/range.rb'

ENV = {} # TODO: Make this not a stub.