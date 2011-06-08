Channel9
--------

Channel9 is a VM written (so far) entirely in Ruby.

What sets it apart from most VMs is that its central construct is
not objects (as in Ruby's VMs), or strings (as in TCL), or hashes
(as in python/perl/etc). The central primitive of channel9 is
continuations (called Channels in the VM's terms).

A channel is a position in code with associated state, including
stack and local variables. With this, you can do some very interesting
things.

Running Channel9
----------------

In its current state, you can run arbitrary channel9 bytecode through
the c9 binary, which loads it from serialized json. From the project
directory:

	> bin/c9 sample/gcd.c9b
	21
	
 