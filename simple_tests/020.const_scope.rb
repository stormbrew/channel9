BLAH = "woop"
module X
  BLAH = "blorp"
  puts BLAH

  class Y
    puts BLAH
    puts ::BLAH
  end
end

puts BLAH
puts X::BLAH

puts X::Y