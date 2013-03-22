# Configuration file for Channel9.rb, based on the default
# 1.8 config included in rubyspec.

class MSpecScript
  # Language features specs
  set :language, [ 'rubyspec/language' ]

  # Core library specs
  set :core, [
    'rubyspec/core',

    # 1.9
    '^rubyspec/core/basicobject'
  ]

  # Standard library specs
  set :library, [
    'rubyspec/library',

    # 1.9 feature
    '^rubyspec/library/cmath',
    '^rubyspec/library/coverage',
    '^rubyspec/library/json',
    '^rubyspec/library/minitest',
    '^rubyspec/library/prime',
    '^rubyspec/library/ripper',
    '^rubyspec/library/rake',
    '^rubyspec/library/rubygems',
  ]

  # An ordered list of the directories containing specs to run
  set :files, get(:language) + get(:core) + get(:library)

  # This set of files is run by mspec ci
  set :ci_files, get(:files)

  # Optional library specs
  set :ffi, 'rubyspec/optional/ffi'

  # A list of _all_ optional library specs
  set :optional, [get(:ffi)]

  # The default implementation to run the specs
  set :target, '../channel9.build/bin/c9.rb'

  set :tags_patterns, [
                        [%r(language/),     'rubyspec/tags/1.8/language/'],
                        [%r(core/),         'rubyspec/tags/1.8/core/'],
                        [%r(command_line/), 'rubyspec/tags/1.8/command_line/'],
                        [%r(library/),      'rubyspec/tags/1.8/library/'],
                        [/_spec.rb$/,       'rubyspec/_tags.txt']
                      ]

  # Enable features
  MSpec.enable_feature :continuation
  #MSpec.enable_feature :fork

  # The Readline specs are not enabled by default because the functionality
  # depends heavily on the underlying library, including whether certain
  # methods are implemented or not. This makes it extremely difficult to
  # make the specs consistently pass. Until a suitable scheme to handle
  # all these issues, the specs will not be enabled by default.
  #
  # MSpec.enable_feature :readline

  if SpecVersion.new(RUBY_VERSION) >= "1.8.7"
    # These are encoding-aware methods backported to 1.8.7+ (eg String#bytes)
    MSpec.enable_feature :encoding_transition
  end
end
