require File.expand_path('../../../spec_helper', __FILE__)
require File.expand_path('../shared/include', __FILE__)

describe "Range#===" do
  it_behaves_like(:range_include, :===)
end
