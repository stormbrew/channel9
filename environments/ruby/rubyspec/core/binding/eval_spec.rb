require File.expand_path('../../../spec_helper', __FILE__)
require File.expand_path('../fixtures/classes', __FILE__)

describe "Binding#eval" do
  ruby_version_is '1.8.7' do
    it "behaves like Kernel.eval(..., self)" do
      obj = BindingSpecs::Demo.new(1)
      bind = obj.get_binding

      bind.eval("@secret += square(3)").should == 10
      bind.eval("a").should be_true

      bind.eval("class Inside; end")
      bind.eval("Inside.name").should == "BindingSpecs::Demo::Inside"
    end

    it "needs to be completed"
  end
end
