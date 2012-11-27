#!/bin/sh
set -e

# This is just a hackjob for now. Will make it better later.

bin/c9 sample/bytecode/gcd-1mil.c9b
bin/c9c sample/c9script/gcd.c9s
bin/c9 sample/c9script/gcd.c9b
