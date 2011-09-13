namespace "git" do
  desc "Update submodules"
  task :submodules do
    sh "git submodule init"
    sh "git submodule update"
  end
end

namespace "build" do
  rule '.c9b' => ['.c9s'] do |t|
    sh "ruby -rubygems -I ../channel9/lib -I ../channel9/ext ../channel9/bin/c9c #{t.source}"
  end

  task :alpha => [
    "environment/kernel/alpha/basic_hash.c9b",
    "environment/kernel/alpha/object.c9b",
    "environment/kernel/alpha/class.c9b",
    "environment/kernel/alpha/module.c9b",
    "environment/kernel/alpha/finish.c9b",
  ]
end
task :build => ["build:alpha"]

namespace "spec" do
  desc "Run simple specs"
  task :simple => ["build"] do
    sh "./simple_tests.rb"
  end


  desc "Run rubyspec language specs"
  task :language => ["build", "git:submodules"] do
    sh "time mspec/bin/mspec :language"
  end
end
task :spec => ["spec:simple", "spec:language"]

task :default => [:spec]
