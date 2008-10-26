#!/usr/bin/env ruby
#--
# Copyright 2006 by Chad Fowler, Rich Kilmer, Jim Weirich and others.
# All rights reserved.
# See LICENSE.txt for permissions.
#++

require 'rubygems'
require 'rubygems/gem_runner'

required_version = Gem::Requirement.new ">= 1.8.3"

unless required_version.satisfied_by? Gem::Version.new(RUBY_VERSION) then
  abort "Expected Ruby Version #{required_version}, was #{RUBY_VERSION}"
end

# We need to preserve the original ARGV to use for passing gem options
# to source gems.  If there is a -- in the line, strip all options after
# it...its for the source building process.
args = !ARGV.include?("--") ? ARGV.clone : ARGV[0...ARGV.index("--")]

Gem::GemRunner.new.run args

