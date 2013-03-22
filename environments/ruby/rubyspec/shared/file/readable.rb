describe :file_readable, :shared => true do
  before :each do
    @file = tmp('i_exist')
    platform_is :windows do
      @file2 = "C:\\windows\\notepad.exe"
    end
    platform_is_not :windows do
      @file2 = "/etc/passwd"
    end
  end

  after :each do
    rm_r @file
  end

  it "returns true if named file is readable by the effective user id of the process, otherwise false" do
    @object.send(@method, @file2).should == true
    File.open(@file,'w') { @object.send(@method, @file).should == true }
  end

  ruby_version_is "1.9" do
    it "accepts an object that has a #to_path method" do
      @object.send(@method, mock_to_path(@file2)).should == true
    end
  end
end

describe :file_readable_missing, :shared => true do
  it "returns false if the file does not exist" do
    @object.send(@method, 'fake_file').should == false
  end
end
