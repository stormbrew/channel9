File.open("/tmp/031.fileio.rb.out", "wb") do |f|
  f.puts "hello"
  f.puts "goodbye"
  f.puts "I don't know why."
end
File.open("/tmp/031.fileio.rb.out", "rb") do |f|
  f.each_line do |line|
    puts line
  end
end
