require 'channel9'

s = Channel9::Stream.new
# we've been invoked with method semantics, so tidy up the work area and
# store our exit path.
s.local_set(:exit) # return handler is our exit
s.pop # we don't care what value was passed in

# define an identity method (returns what it's given)
s.channel_new(:identity_method)
s.local_set(:identity)
s.jmp(:identity_method_done)
s.set_label(:identity_method) # start method, stack will have message and return address
s.local_set(:output)
s.message_unpack(1, 0, 0)
s.pop # unpack leaves the message on the stack, we're done with it.
s.local_set(:value)
s.local_get(:output)
s.local_get(:value)
s.channel_ret # send back what we got.
s.set_label(:identity_method_done)

# call the identity method with constant 1 and store its result
s.local_get(:identity)
s.push(1)
s.message_new(:identity, 1)
s.channel_call
s.pop
s.local_set(:x)

# print the result of the identity method
s.channel_special(:stdout)
s.local_get(:x)
s.channel_call
# clean up here.
s.pop
s.pop

# properly call the exit handler
s.local_get(:exit)
s.push(nil)
s.channel_ret


Channel9::Environment.new(Channel9::Context.new(s)).run