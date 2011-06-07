require 'channel9'

s = Channel9::Stream.new
# we've been invoked with method semantics, so tidy up the work area and
# store our exit path.
s.set_local(:exit) # return handler is our exit
s.pop # we don't care what value was passed in

# define an identity method (returns what it's given)
s.new_channel(:identity_method)
s.set_local(:identity)
s.jmp(:identity_method_done)
s.set_label(:identity_method) # start method, stack will have input value and return address
s.special_channel(:InvalidReturn) # push a new return value (that will die if returned to)
s.channel_send # and send back what we got.
s.set_label(:identity_method_done)

# call the identity method with constant 1 and store its result
s.push(1)
s.get_local(:identity)
s.new_channel(:set_x)
s.channel_send
s.set_label(:set_x)
s.pop
s.set_local(:x)

# print the result of the identity method
s.get_local(:x)
s.special_channel(:Stdout)
s.new_channel(:done_print)
s.channel_send
s.set_label(:done_print)
# clean up here.
s.pop
s.pop

# properly call the exit handler
s.push(nil)
s.get_local(:exit)
s.special_channel(:InvalidReturn)
s.channel_send


Channel9::Environment.new(Channel9::Context.new(s)).run