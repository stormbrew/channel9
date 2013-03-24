# needed for tagging to work
puts "blah\\nblah\\nblorp".gsub(/\\n/, "\n")
puts "blah\nblah\nblorp".gsub(/\n/, "\\n")
