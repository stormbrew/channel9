require 'channel9'

s = Channel9::Stream.new
s.new_channel(:identity_method)
s.set_local(:identity)
s.jmp(:identity_method_done)
s.set_label(:identity_method)
s.special_channel(:InvalidReturn)
s.channel_send
s.set_label(:identity_method_done)

s.push(1)
s.get_local(:identity)
s.new_channel(:set_x)
s.channel_send
s.set_label(:set_x)
s.pop
s.set_local(:x)

s.get_local(:x)
s.special_channel(:Stdout)
s.new_channel(:done_print)
s.channel_send
s.set_label(:done_print)
s.pop
s.pop



Channel9::Environment.new(Channel9::Context.new(s)).run