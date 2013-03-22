# For now, just enough to run mspec.
module RbConfig
  CONFIG = {}
  CONFIG['bindir'] = File.expand_path("../../../bin", __FILE__)
  CONFIG['ruby_install_name'] = "c9.rb"
end