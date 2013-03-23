require File.expand_path('../../spec_helper', __FILE__)
require File.expand_path('../fixtures/classes', __FILE__)

describe "Literal Regexps" do
  it "matches against $_ (last input) in a conditional if no explicit matchee provided" do
    $_ = nil

    (true if /foo/).should_not == true

    $_ = "foo"

    (true if /foo/).should == true
  end

  it "yields a Regexp" do
    /Hello/.should be_kind_of(Regexp)
  end

  it "caches the Regexp object" do
    rs = []
    2.times do |i|
      x = 1
      rs << /foo/
    end
    rs[0].should equal(rs[1])
  end

  it "throws SyntaxError for malformed literals" do
    lambda { eval('/(/') }.should raise_error(SyntaxError)
  end

  #############################################################################
  # %r
  #############################################################################

  it "supports paired delimiters with %r" do
    LanguageSpecs.paired_delimiters.each do |p0, p1|
      eval("%r#{p0} foo #{p1}").should == / foo /
    end
  end

  it "supports grouping constructs that are also paired delimiters" do
    LanguageSpecs.paired_delimiters.each do |p0, p1|
      eval("%r#{p0} () [c]{1} #{p1}").should == / () [c]{1} /
    end
  end

  it "allows second part of paired delimiters to be used as non-paired delimiters" do
    LanguageSpecs.paired_delimiters.each do |p0, p1|
      eval("%r#{p1} foo #{p1}").should == / foo /
    end
  end

  it "disallows first part of paired delimiters to be used as non-paired delimiters" do
    LanguageSpecs.paired_delimiters.each do |p0, p1|
      lambda { eval("%r#{p0} foo #{p0}") }.should raise_error(SyntaxError)
    end
  end

  it "supports non-paired delimiters delimiters with %r" do
    LanguageSpecs.non_paired_delimiters.each do |c|
      eval("%r#{c} foo #{c}").should == / foo /
    end
  end

  it "disallows alphabets as non-paired delimiter with %r" do
    lambda { eval('%ra foo a') }.should raise_error(SyntaxError)
  end

  it "disallows spaces after %r and delimiter" do
    lambda { eval('%r !foo!') }.should raise_error(SyntaxError)
  end

  it "allows unescaped / to be used with %r" do
    %r[/].to_s.should == /\//.to_s
  end


  #############################################################################
  # Specs for the matching semantics
  #############################################################################

  it 'supports . (any character except line terminator)' do
    # Basic matching
    /./.match("foo").to_a.should == ["f"]
    # Basic non-matching
    /./.match("").should be_nil
    /./.match("\n").should be_nil
    /./.match("\0").to_a.should == ["\0"]
  end


  it 'supports | (alternations)' do
    /a|b/.match("a").to_a.should == ["a"]
  end

  it 'supports (?> ) (embedded subexpression)' do
    /(?>foo)(?>bar)/.match("foobar").to_a.should == ["foobar"]
    /(?>foo*)obar/.match("foooooooobar").should be_nil # it is possesive
  end

  it 'supports (?# )' do
    /foo(?#comment)bar/.match("foobar").to_a.should == ["foobar"]
    /foo(?#)bar/.match("foobar").to_a.should == ["foobar"]
  end
end

language_version __FILE__, "regexp"
