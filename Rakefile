namespace "git" do
  desc "Update submodules"
  task :submodules do
    sh "git submodule init"
    sh "git submodule update"
  end
end

namespace "spec" do
  desc "Run simple specs"
  task :simple do
    sh "./simple_tests.rb"
  end


  desc "Run rubyspec language specs"
  task :language => ["git:submodules"] do
    sh "time mspec/bin/mspec :language"
  end
end
task :spec => ["spec:simple", "spec:language"]

task :default => [:spec]
