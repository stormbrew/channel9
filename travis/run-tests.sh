#!/bin/sh
set -e

# This is just a hackjob for now. Will make it better later.
export PATH=${PATH}:${PWD}/build/bin:${PWD}/bin

# Make libffi pkgconfig setup available.
[ "${TRAVIS_OS_NAME}" = "osx" ] && {
	brew --prefix libffi
	ls $(brew --prefix libffi)/lib/pkgconfig
	export PKG_CONFIG_PATH="$(brew --prefix libffi)/lib/pkgconfig/":$PKG_CONFIG_PATH
}

c9 sample/bytecode/gcd-1mil.c9b
c9 sample/c9script/gcd.c9s

cd environments/ruby
# TODO: Make this rake spec when we can do tags.
rake spec
