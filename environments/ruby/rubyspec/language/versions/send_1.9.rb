# FIXME: Add error case
#
require File.expand_path("../../fixtures/send_1.9.rb", __FILE__)

specs = LangSendSpecs

describe "Invoking a method" do
  describe "with required args after the rest arguments" do
    it "binds the required arguments first" do
      specs.fooM0RQ1(1).should == [[], 1]
      specs.fooM0RQ1(1,2).should == [[1], 2]
      specs.fooM0RQ1(1,2,3).should == [[1,2], 3]

      specs.fooM1RQ1(1,2).should == [1, [], 2]
      specs.fooM1RQ1(1,2,3).should == [1, [2], 3]
      specs.fooM1RQ1(1,2,3,4).should == [1, [2, 3], 4]

      specs.fooM1O1RQ1(1,2).should == [1, 9, [], 2]
      specs.fooM1O1RQ1(1,2,3).should == [1, 2, [], 3]
      specs.fooM1O1RQ1(1,2,3,4).should == [1, 2, [3], 4]

      specs.fooM1O1RQ2(1,2,3).should == [1, 9, [], 2, 3]
      specs.fooM1O1RQ2(1,2,3,4).should == [1, 2, [], 3, 4]
      specs.fooM1O1RQ2(1,2,3,4,5).should == [1, 2, [3], 4, 5]
    end
  end

  describe "with manditory arguments after optional arguments" do
    it "binds the required arguments first" do
      specs.fooO1Q1(0,1).should == [0,1]
      specs.fooO1Q1(2).should == [1,2]

      specs.fooM1O1Q1(2,3,4).should == [2,3,4]
      specs.fooM1O1Q1(1,3).should == [1,2,3]

      specs.fooM2O1Q1(1,2,4).should == [1,2,3,4]

      specs.fooM2O2Q1(1,2,3,4,5).should == [1,2,3,4,5]
      specs.fooM2O2Q1(1,2,3,5).should == [1,2,3,4,5]
      specs.fooM2O2Q1(1,2,5).should == [1,2,3,4,5]

      specs.fooO4Q1(1,2,3,4,5).should == [1,2,3,4,5]
      specs.fooO4Q1(1,2,3,5).should == [1,2,3,4,5]
      specs.fooO4Q1(1,2,5).should == [1,2,3,4,5]
      specs.fooO4Q1(1,5).should == [1,2,3,4,5]
      specs.fooO4Q1(5).should == [1,2,3,4,5]

      specs.fooO4Q2(1,2,3,4,5,6).should == [1,2,3,4,5,6]
      specs.fooO4Q2(1,2,3,5,6).should == [1,2,3,4,5,6]
      specs.fooO4Q2(1,2,5,6).should == [1,2,3,4,5,6]
      specs.fooO4Q2(1,5,6).should == [1,2,3,4,5,6]
      specs.fooO4Q2(5,6).should == [1,2,3,4,5,6]
    end
  end

  it "with .() invokes #call" do
    q = proc { |z| z }
    q.(1).should == 1

    obj = mock("paren call")
    obj.should_receive(:call).and_return(:called)
    obj.().should == :called
  end

  it "allows a vestigial trailing ',' in the arguments" do
    specs.fooM1(1,).should == [1]
  end

  it "with splat operator attempts to coerce it to an Array if the object respond_to?(:to_a)" do
    ary = [2,3,4]
    obj = mock("to_a")
    obj.should_receive(:to_a).and_return(ary).twice
    specs.fooM0R(*obj).should == ary
    specs.fooM1R(1,*obj).should == [1, ary]
  end

  it "with splat operator * and non-Array value uses value unchanged if it does not respond_to?(:to_ary)" do
    obj = Object.new
    obj.should_not respond_to(:to_a)

    specs.fooM0R(*obj).should == [obj]
    specs.fooM1R(1,*obj).should == [1, [obj]]
  end

  it "accepts additional arguments after splat expansion" do
    a = [1,2]
    specs.fooM4(*a,3,4).should == [1,2,3,4]
    specs.fooM4(0,*a,3).should == [0,1,2,3]
  end

  it "accepts multiple splat expansions in the same argument list" do
    a = [1,2,3]
    b = 7
    c = mock("pseudo-array")
    c.should_receive(:to_a).and_return([0,0])

    d = [4,5]
    specs.rest_len(*a,*d,6,*b).should == 7
    specs.rest_len(*a,*a,*a).should == 9
    specs.rest_len(0,*a,4,*5,6,7,*c,-1).should == 11
  end

  it "expands an array to arguments grouped in parentheses" do
    specs.destructure2([40,2]).should == 42
  end

  it "expands an array to arguments grouped in parentheses and ignores any rest arguments in the array" do
    specs.destructure2([40,2,84]).should == 42
  end

  it "expands an array to arguments grouped in parentheses and sets not specified arguments to nil" do
    specs.destructure2b([42]).should == [42, nil]
  end

  it "expands an array to arguments grouped in parentheses which in turn takes rest arguments" do
    specs.destructure4r([1, 2, 3]).should == [1, 2, [], 3, nil]
    specs.destructure4r([1, 2, 3, 4]).should == [1, 2, [], 3, 4]
    specs.destructure4r([1, 2, 3, 4, 5]).should == [1, 2, [3], 4, 5]
  end
end
