require File.expand_path('../../../spec_helper', __FILE__)
require File.expand_path('../fixtures/common', __FILE__)

ruby_version_is "1.9" do
  describe "BasicObject" do
    it "raises NoMethodError for nonexistent methods after #method_missing is removed" do
      script = fixture __FILE__, "remove_method_missing.rb"
      ruby_exe(script).chomp.should == "NoMethodError"
    end
  end

  describe "BasicObject metaclass" do
    before :each do
      @meta = class << BasicObject; self; end
    end

    it "is an instance of Class" do
      @meta.should be_an_instance_of(Class)
    end

    it "has Class as superclass" do
      @meta.superclass.should equal(Class)
    end

    it "contains methods for the BasicObject class" do
      @meta.class_eval do
        def rubyspec_test_method() :test end
      end

      BasicObject.rubyspec_test_method.should == :test
    end
  end

  describe "BasicObject instance metaclass" do
    before :each do
      @object = BasicObject.new
      @meta = class << @object; self; end
    end

    it "is an instance of Class" do
      @meta.should be_an_instance_of(Class)
    end

    it "has BasicObject as superclass" do
      @meta.superclass.should equal(BasicObject)
    end

    it "contains methods defined for the BasicObject instance" do
      @meta.class_eval do
        def test_method() :test end
      end

      @object.test_method.should == :test
    end
  end

  describe "BasicObject subclass" do
    it "contains Kernel methods when including Kernel" do
      obj = BasicObjectSpecs::BOSubclass.new

      obj.instance_variable_set(:@test, :value)
      obj.instance_variable_get(:@test).should == :value

      obj.send(:hash).should be_an_instance_of(Fixnum)
    end
  end
end
