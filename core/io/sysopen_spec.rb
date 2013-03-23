require File.expand_path('../../../spec_helper', __FILE__)

describe "IO.sysopen" do

  before :all do
    @filename = tmp("rubinius-spec-io-sysopen-#{$$}.txt")
  end

  before :each do
    @fd = nil
  end

  after :each do
    IO.for_fd(@fd).close if @fd
  end

  after :all do
    File.unlink @filename
  end

  it "returns the file descriptor for a given path" do
    @fd = IO.sysopen(@filename, "w")
    @fd.should be_kind_of(Fixnum)
    @fd.should_not equal(0)
  end

  # opening a directory is not supported on Windows
  platform_is_not :windows do
    it "works on directories" do
      @fd = IO.sysopen(tmp(""))    # /tmp
      @fd.should be_kind_of(Fixnum)
      @fd.should_not equal(0)
    end
  end

  ruby_version_is "1.9" do
    it "calls #to_path on first argument" do
      p = mock('path')
      p.should_receive(:to_path).and_return(@filename)
      @fd = IO.sysopen(p, 'w')
    end
  end

  it "accepts a mode as second argument" do
    @fd = 0
    lambda { @fd = IO.sysopen(@filename, "w") }.should_not raise_error
    @fd.should_not equal(0)
  end

  it "accepts permissions as third argument" do
    @fd = IO.sysopen(@filename, "w", 777)
    @fd.should_not equal(0)
  end
end
