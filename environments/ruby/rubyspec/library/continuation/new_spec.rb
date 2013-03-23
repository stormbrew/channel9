require File.expand_path('../../../spec_helper', __FILE__)
require File.expand_path('../../../shared/continuation/new', __FILE__)

with_feature :continuation_library do
  require 'continuation'

  describe "Continuation.new" do
    it_behaves_like :continuation_new, :new
  end
end
