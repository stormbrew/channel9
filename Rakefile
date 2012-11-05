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
  rule '.c9b' => ['.rb'] do |t|
    sh "ruby bin/c9.rb -c #{t.source}"
  end

  task :alpha => [
    "environment/kernel/alpha/basic_hash.c9b",
    "environment/kernel/alpha/object.c9b",
    "environment/kernel/alpha/class.c9b",
    "environment/kernel/alpha/module.c9b",
    "environment/kernel/alpha/finish.c9b",
  ]

  task :beta => [
    "environment/kernel/beta/array.c9b",
    "environment/kernel/beta/class.c9b",
    "environment/kernel/beta/enumerable.c9b",
    "environment/kernel/beta/exceptions.c9b",
    "environment/kernel/beta/kernel.c9b",
    "environment/kernel/beta/module.c9b",
    "environment/kernel/beta/proc.c9b",
    "environment/kernel/beta/singletons.c9b",
    "environment/kernel/beta/static_tuple.c9b",
    "environment/kernel/beta/string.c9b",
    "environment/kernel/beta/symbol.c9b",
    "environment/kernel/beta/tuple.c9b",
  ]

  task :delta => [
    "environment/kernel/delta/channel9.c9b",
    "environment/kernel/delta/class.c9b",
    "environment/kernel/delta/comparable.c9b",
    "environment/kernel/delta/dir.c9b",
    "environment/kernel/delta/io.c9b",
    "environment/kernel/delta/file.c9b",
    "environment/kernel/delta/fixnum.c9b",
    "environment/kernel/delta/float.c9b",
    "environment/kernel/delta/hash.c9b",
    "environment/kernel/delta/kernel.c9b",
    "environment/kernel/delta/message.c9b",
    "environment/kernel/delta/module.c9b",
    "environment/kernel/delta/object.c9b",
    "environment/kernel/delta/range.c9b",
    "environment/kernel/delta/regexp.c9b",
    "environment/kernel/delta/signal.c9b",
    "environment/kernel/delta/string.c9b",
    "environment/kernel/delta/table.c9b",
    "environment/kernel/delta/thread.c9b",
    "environment/kernel/delta/time.c9b",
    "environment/kernel/delta.c9b",
  ]
end
task :build => ["build:alpha", "build:beta", "build:delta"]

namespace "spec" do
  desc "Run simple specs"
  task :simple => ["build"] do
    sh "bin/simple_test_runner.rb simple_tests"
  end

  task :perf => ["build"] do
    sh "bin/simple_test_runner.rb perf_tests"
  end

  desc "Run rubyspec language specs"
  task :language => ["build", "git:submodules"] do
    sh "time mspec/bin/mspec :language"
  end
end
task :spec => ["spec:simple", "spec:language"]

task :default => [:spec]
