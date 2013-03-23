require File.expand_path('../../../spec_helper', __FILE__)
require File.expand_path('../fixtures/classes', __FILE__)

describe "Class#dup" do
  it "duplicates both the class and the singleton class" do
    klass = Class.new do
      def hello
        "hello"
      end

      def self.message
        "text"
      end
    end

    klass_dup = klass.dup

    klass_dup.new.hello.should == "hello"
    klass_dup.message.should == "text"
  end

  it "retains the correct ancestor chain for the singleton class" do
    super_klass = Class.new do
      def hello
        "hello"
      end

      def self.message
        "text"
      end
    end

    klass = Class.new(super_klass)
    klass_dup = klass.dup

    klass_dup.new.hello.should == "hello"
    klass_dup.message.should == "text"
  end

  ruby_version_is ""..."1.9" do
    it "sets the name from the class to \"\" if not assigned to a constant" do
      copy = CoreClassSpecs::Record.dup
      copy.name.should == ""
    end
  end

  ruby_version_is "1.9" do
    it "sets the name from the class to nil if not assigned to a constant" do
      copy = CoreClassSpecs::Record.dup
      copy.name.should be_nil
    end
  end

  it "stores the new name if assigned to a constant" do
    CoreClassSpecs::RecordCopy = CoreClassSpecs::Record.dup
    CoreClassSpecs::RecordCopy.name.should == "CoreClassSpecs::RecordCopy"
  end

end
