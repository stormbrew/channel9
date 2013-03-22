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
    sh "bin/simple_test_runner.rb simple_tests"
  end

  task :perf do
    sh "bin/simple_test_runner.rb perf_tests"
  end

  desc "Run rubyspec language specs"
  task :language => ["git:submodules"] do
    sh "time mspec/bin/mspec -f specdoc :language"
  end
end
task :spec => ["spec:simple", "spec:language"]

task :default => [:spec]
