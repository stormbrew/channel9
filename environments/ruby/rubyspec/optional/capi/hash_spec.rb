require File.expand_path('../spec_helper', __FILE__)

load_extension("hash")

describe "C-API Hash function" do
  before :each do
    @s = CApiHashSpecs.new
  end

  describe "rb_hash" do
    it "calls #hash on the object" do
      obj = mock("rb_hash")
      obj.should_receive(:hash).and_return(5)
      @s.rb_hash(obj).should == 5
    end

    ruby_version_is "1.8.7" do
      it "converts a Bignum returned by #hash to a Fixnum" do
        obj = mock("rb_hash bignum")
        obj.should_receive(:hash).and_return(bignum_value())

        # The actual conversion is an implementation detail.
        # We only care that ultimately we get a Fixnum instance.
        @s.rb_hash(obj).should be_an_instance_of(Fixnum)
      end

      it "calls #to_int to converts a value returned by #hash to a Fixnum" do
        obj = mock("rb_hash to_int")
        obj.should_receive(:hash).and_return(obj)
        obj.should_receive(:to_int).and_return(12)

        @s.rb_hash(obj).should == 12
      end

      it "raises a TypeError if the object does not implement #to_int" do
        obj = mock("rb_hash no to_int")
        obj.should_receive(:hash).and_return(nil)

        lambda { @s.rb_hash(obj) }.should raise_error(TypeError)
      end
    end
  end

  describe "rb_hash_new" do
    it "returns a new hash" do
      @s.rb_hash_new.should == {}
    end

    it "creates a hash with no default proc" do
      @s.rb_hash_new {}.default_proc.should be_nil
    end
  end

  describe "rb_hash_aref" do
    it "returns the value associated with the key" do
      hsh = {:chunky => 'bacon'}
      @s.rb_hash_aref(hsh, :chunky).should == 'bacon'
    end

    it "returns the default value if it exists" do
      hsh = Hash.new(0)
      @s.rb_hash_aref(hsh, :chunky).should == 0
      @s.rb_hash_aref_nil(hsh, :chunky).should be_false
    end

    it "returns nil if the key does not exist" do
      hsh = { }
      @s.rb_hash_aref(hsh, :chunky).should be_nil
      @s.rb_hash_aref_nil(hsh, :chunky).should be_true
    end
  end

  describe "rb_hash_aset" do
    it "adds the key/value pair and returns the value" do
      hsh = {}
      @s.rb_hash_aset(hsh, :chunky, 'bacon').should == 'bacon'
      hsh.should == {:chunky => 'bacon'}
    end
  end

  describe "rb_hash_delete" do
    it "removes the key and returns the value" do
      hsh = {:chunky => 'bacon'}
      @s.rb_hash_delete(hsh, :chunky).should == 'bacon'
      hsh.should == {}
    end
  end

  describe "rb_hash_delete_if" do
    it "removes an entry if the block returns true" do
      h = { :a => 1, :b => 2, :c => 3 }
      @s.rb_hash_delete_if(h) { |k, v| v == 2 }
      h.should == { :a => 1, :c => 3 }
    end

    ruby_version_is ""..."1.8.7" do
      it "raises a LocalJumpError when no block is passed" do
        lambda { @s.rb_hash_delete_if({:a => 1}) }.should raise_error(LocalJumpError)
      end
    end

    ruby_version_is "1.8.7" do
      it "returns an Enumerator when no block is passed" do
        @s.rb_hash_delete_if({:a => 1}).should be_an_instance_of(enumerator_class)
      end
    end
  end

  describe "rb_hash_foreach" do
    it "iterates over the hash" do
      hsh = {:name => "Evan", :sign => :libra}

      out = @s.rb_hash_foreach(hsh)
      out.equal?(hsh).should == false
      out.should == hsh
    end

    it "stops via the callback" do
      hsh = {:name => "Evan", :sign => :libra}

      out = @s.rb_hash_foreach_stop(hsh)
      out.size.should == 1
    end

    it "deletes via the callback" do
      hsh = {:name => "Evan", :sign => :libra}

      out = @s.rb_hash_foreach_delete(hsh)
      out.should == {:name => "Evan", :sign => :libra}
      hsh.should == {}
    end
  end

  # rb_hash_size is a static symbol in MRI
  extended_on :rubinius do
    describe "rb_hash_size" do
      it "returns the size of the hash" do
        hsh = {:fast => 'car', :good => 'music'}
        @s.rb_hash_size(hsh).should == 2
      end

      it "returns zero for an empty hash" do
        @s.rb_hash_size({}).should == 0
      end
    end

    # TODO: make this shared so it runs on 1.8.7
    describe "rb_hash_lookup" do
      it "returns the value associated with the key" do
        hsh = {:chunky => 'bacon'}
        @s.rb_hash_lookup(hsh, :chunky).should == 'bacon'
      end

      it "does not return the default value if it exists" do
        hsh = Hash.new(0)
        @s.rb_hash_lookup(hsh, :chunky).should be_nil
        @s.rb_hash_lookup_nil(hsh, :chunky).should be_true
      end

      it "returns nil if the key does not exist" do
        hsh = { }
        @s.rb_hash_lookup(hsh, :chunky).should be_nil
        @s.rb_hash_lookup_nil(hsh, :chunky).should be_true
      end
    end
  end
end
