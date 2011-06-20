# symbol, string, enumerable, tuple, array, and kernel loaded by environment.

$LOAD_PATH = Array.new($LOAD_PATH)
$: = $LOAD_PATH

load 'boot/object.rb'
load 'boot/class.rb'
load 'boot/module.rb'

load 'boot/fixnum.rb'
load 'boot/message.rb'
load 'boot/table.rb'

load 'boot/hash.rb'

load 'boot/file.rb'
