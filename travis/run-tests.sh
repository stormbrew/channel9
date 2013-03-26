#!/bin/sh
set -e

# This is just a hackjob for now. Will make it better later.
export PATH=${PATH}:${PWD}/build/bin:${PWD}/bin

c9 sample/bytecode/gcd-1mil.c9b
bin/c9c sample/c9script/gcd.c9s
c9 sample/c9script/gcd.c9b

cd environments/ruby
# TODO: Make this rake spec when we can do tags.
rake spec
