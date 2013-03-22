require File.expand_path('../../../spec_helper', __FILE__)
require File.expand_path('../fixtures/classes', __FILE__)

# FIXME: These methods exist on 1.9 only when the -n or -p option is passed to
# ruby, but we currently don't have a way of specifying that.
ruby_version_is ""..."1.9" do
  describe "Kernel#chomp" do
    it "is a private method" do
      Kernel.should have_private_instance_method(:chomp)
    end
  end

  describe "Kernel#chomp!" do
    it "is a private method" do
      Kernel.should have_private_instance_method(:chomp!)
    end
  end

  describe "Kernel.chomp" do
    it "needs to be reviewed for spec completeness"
  end

  describe "Kernel.chomp!" do
    it "needs to be reviewed for spec completeness"
  end
end
