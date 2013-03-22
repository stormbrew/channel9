require File.expand_path('../../../spec_helper', __FILE__)
require File.expand_path('../../../shared/complex/numeric/real', __FILE__)

ruby_version_is "1.9" do
  describe "Numeric#real" do
    it_behaves_like(:numeric_real, :real)
  end
end

ruby_version_is "1.9" do
  describe "Numeric#real?" do
    it "needs to be reviewed for spec completeness"
  end
end
