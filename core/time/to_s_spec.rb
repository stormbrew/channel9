require File.expand_path('../../../spec_helper', __FILE__)
require File.expand_path('../fixtures/methods', __FILE__)
require File.expand_path('../shared/inspect', __FILE__)

describe "Time.to_s" do
  it_behaves_like :inspect, :to_s
end

describe "Time#to_s" do
  it "needs to be reviewed for spec completeness"
end
