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

Building
--------

You will need to have cmake, relatively modern gcc, and a ruby install
in order to build Channel9. You can follow the following steps to build it:

	> git clone git://github.com/stormbrew/channel9.git
	> mkdir channel9.build
	> cd channel9.build
	> cmake ../channel9
	> make

Running
-------

Assuming that was successful, you can use the generated bin/c9 exe to
run some bytecode files:

	> bin/c9 ../channel9/samples/gcd.c9b

Channel9.rb
-----------

To also build the work in progress ruby VM on top of Channel9, you can
do the following:

	> pushd ../channel9/environments
	> git clone git://github.com/stormbrew/channel9.rb.git
	> popd
	> make

At which point you can run ruby scripts like so:

	> bin/c9 ../channel9/environments/channel9.rb/simple_tests/001.math.rb

Debugging
---------

You can pass flags to CMake to make it build a more debugger-friendly VM:

	> cmake -DCMAKE_BUILD_TYPE=Debug ../channel9

Or a more valgrindable VM:

	> cmake -DENABLE_VALGRIND ../channel9

Or you can set trace levels (SPAM, DEBUG, INFO, WARN, ERROR, CRIT, or OFF) for
the four trace facilities (GENERAL, VM, ALLOC, and GC):

	> cmake -DTRACE_LEVEL_GC=INFO -DTRACE_LEVEL_VM=OFF -DTRACE_LEVEL_GENERAL=WARN

Note: They all default to off, and you need to pass -TT to bin/c9 to enable
tracing at startup. -T will advise the environment to trace after it has loaded
itself (thus not spamming you with noise while it boots up).

Or you can combine any set of those.
