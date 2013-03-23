require 'date'
require File.expand_path('../../../spec_helper', __FILE__)

describe "Date constants" do

  it "defines ITALY" do
    Date::ITALY.should == 2299161 # 1582-10-15
  end

  it "defines ENGLAND" do
    Date::ENGLAND.should == 2361222 # 1752-09-14
  end

  # Fixes in 1.8.7
  ruby_bug "#", "1.8.6" do
    it "defines JULIAN" do
      (Date::JULIAN <=> Date::Infinity.new).should == 0
    end
  end

  # Fixed in 1.8.7
  ruby_bug "#", "1.8.6" do
    it "defines GREGORIAN" do
      (Date::GREGORIAN <=> -Date::Infinity.new).should == 0
    end
  end

  it "defines MONTHNAMES" do
    Date::MONTHNAMES.should == [nil] + %w(January February March April May June July
                                          August September October November December)
  end

  it "defines DAYNAMES" do
    Date::DAYNAMES.should == %w(Sunday Monday Tuesday Wednesday Thursday Friday Saturday)
  end

  it "defines ABBR_MONTHNAMES" do
    Date::ABBR_DAYNAMES.should == %w(Sun Mon Tue Wed Thu Fri Sat)
  end

end
