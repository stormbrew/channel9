require File.expand_path('../../../spec_helper', __FILE__)
require File.expand_path('../fixtures/classes', __FILE__)
require File.expand_path('../shared/exit', __FILE__)

describe "Thread#kill" do
  it_behaves_like :thread_exit, :kill
end

describe "Thread#kill!" do
  it "needs to be reviewed for spec completeness"
end

describe "Thread.kill" do
  it "needs to be reviewed for spec completeness"
end
