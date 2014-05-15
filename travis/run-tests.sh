#!/bin/sh
set -e

rvm use ${RUBY_VERSION}

# This is just a hackjob for now. Will make it better later.
export PATH=${PATH}:${PWD}/build/bin:${PWD}/bin
export LD_LIBRARY_PATH=$(ruby -e 'require "rbconfig"; puts RbConfig::CONFIG["prefix"]'):${LD_LIBRARY_PATH}

c9 sample/bytecode/gcd-1mil.c9b
c9 sample/c9script/gcd.c9s

cd environments/ruby
# TODO: Make this rake spec when we can do tags.
rake spec
