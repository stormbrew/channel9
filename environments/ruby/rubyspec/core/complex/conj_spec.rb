require File.expand_path('../../../spec_helper', __FILE__)

ruby_version_is "1.9" do

  require File.expand_path('../../../shared/complex/conjugate', __FILE__)

  describe "Complex#conj" do
    it_behaves_like(:complex_conjugate, :conj)
  end
end
