module Dir
  def self.pwd
    "." # TODO: Make not a stub.
  end
  def self.chdir(path)
    $__c9_ffi_chdir.call(path.to_s_prim)
  end
  def self.glob(names)
    $__c9_glob.call(names.to_s_prim).collect {|i| i.to_s }.to_a
  end
  def self.[](names)
    glob(names)
  end
end
