Channel9
--------

Channel9 is a multilanguage VM designed for dynamic languages to be built on.
What separates it from most other similar endeavors is the flexibility of the
runtime in terms of execution flow. The VM allows the language implementation
to save execution state through continuations in order to allow it to control
its threading model, error model, etc.

Moreover it does not require a linear stack at all, so it's a good basis to
experiment with execution models in general. Although it is a stack-based VM,
the stack is local only to the executing method.

For more information, please check out the documentation in the doc dir (TODO: when
there is some...).

Dependencies
------------

You need to have the following things installed and available to build
and run channel9:

 * CMake (apt-get install cmake)
 * GCC or clang (apt-get install build-essentials)
 * libffi (apt-get install libffi-dev)
 * Ruby (apt-get install ruby or get RVM)
 * The bundler gem for ruby (gem install bundler)

If you want to run the channel9.rb test suite you will probably also want
to have an rvm install, since it uses rvm to ensure that it's testing against
the correct version of ruby (1.8.7).

Building
--------

You will need to have cmake, relatively modern gcc, and a ruby install with
bundler in order to build Channel9. You can follow the following steps to build
it:

	> git clone git://github.com/stormbrew/channel9.git
	> cd channel9
	> bundle install
	> mkdir build
	> cd build
	> cmake ..
	> make

Running
-------

Assuming that was successful, you can use the generated bin/c9 exe to
run some bytecode files:

	> bin/c9 ../samples/gcd.c9b

Channel9.rb
-----------

The work in progress ruby VM should also have build, you can
do the following to try it out:

	> bin/c9 ../environments/channel9.rb/simple_tests/001.math.rb

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
