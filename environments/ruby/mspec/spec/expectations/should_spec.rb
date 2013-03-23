require File.dirname(__FILE__) + '/../spec_helper'
require 'rbconfig'

describe MSpec do
  before :all do
    path = RbConfig::CONFIG['bindir']
    exe  = RbConfig::CONFIG['ruby_install_name']
    file = File.dirname(__FILE__) + '/should.rb'
    @out = `#{path}/#{exe} #{file}`
  end

  describe "#should" do
    it "records failures" do
      @out.should =~ Regexp.new(Regexp.escape(%[
1)
MSpec expectation method #should causes a failue to be recorded FAILED
Expected 1
 to equal 2
]))
    end

    it "raises exceptions for examples with no expectations" do
      @out.should =~ Regexp.new(Regexp.escape(%[
2)
MSpec expectation method #should registers that an expectation has been encountered FAILED
No behavior expectation was found in the example
]))
    end
  end

  describe "#should_not" do
    it "records failures" do
      @out.should =~ Regexp.new(Regexp.escape(%[
3)
MSpec expectation method #should_not causes a failure to be recorded FAILED
Expected 1
 not to equal 1
]))
    end

    it "raises exceptions for examples with no expectations" do
      @out.should =~ Regexp.new(Regexp.escape(%[
4)
MSpec expectation method #should_not registers that an expectation has been encountered FAILED
No behavior expectation was found in the example
]))
    end
  end

  it "prints status information" do
    @out.should =~ /\.FF\.\.FF\./
  end

  it "prints out a summary" do
    @out.should =~ /0 files, 8 examples, 6 expectations, 4 failures, 0 errors/
  end

  it "records expectations" do
    @out.should =~ /I was called 6 times/
  end
end
