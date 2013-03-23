describe "The -x command line option" do
  before :each do
    @file = fixture __FILE__, "embedded_ruby.txt"
  end

  it "runs code after the first /\#!.*ruby.*/-ish line in target file" do
    result = `#{RUBY_EXE} -x #{@file}`
    result.should == "success\n"
  end
  
  it "needs to be reviewed for spec completeness"
end
